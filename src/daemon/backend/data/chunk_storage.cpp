/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <daemon/backend/data/chunk_storage.hpp>
#include <global/path_util.hpp>

#include <cerrno>
#include <utility>

#include <boost/filesystem.hpp>
#include <spdlog/spdlog.h>

extern "C" {
#include <sys/statfs.h>
}

namespace bfs = boost::filesystem;
using namespace std;

namespace gkfs {
namespace data {

// private functions

string ChunkStorage::absolute(const string& internal_path) const {
    assert(gkfs::path::is_relative(internal_path));
    return fmt::format("{}/{}", root_path_, internal_path);
}

string ChunkStorage::get_chunks_dir(const string& file_path) {
    assert(gkfs::path::is_absolute(file_path));
    string chunk_dir = file_path.substr(1);
    ::replace(chunk_dir.begin(), chunk_dir.end(), '/', ':');
    return chunk_dir;
}

string ChunkStorage::get_chunk_path(const string& file_path, gkfs::rpc::chnk_id_t chunk_id) {
    return fmt::format("{}/{}", get_chunks_dir(file_path), chunk_id);
}

/**
 * Creates a chunk directories are all chunk files are placed in.
 * The path to the real file will be used as the directory name
 * @param file_path
 * @returns 0 on success or errno on failure
 */
void ChunkStorage::init_chunk_space(const string& file_path) const {
    auto chunk_dir = absolute(get_chunks_dir(file_path));
    auto err = mkdir(chunk_dir.c_str(), 0750);
    if (err == -1 && errno != EEXIST) {
        auto err_str = fmt::format("{}() Failed to create chunk directory. File: '{}', Error: '{}'", __func__,
                                   file_path, errno);
        throw ChunkStorageException(errno, err_str);
    }
}

// public functions

/**
 *
 * @param path
 * @param chunksize
 * @throws ChunkStorageException
 */
ChunkStorage::ChunkStorage(string path, const size_t chunksize) :
        root_path_(std::move(path)),
        chunksize_(chunksize) {
    /* Initialize logger */
    log_ = spdlog::get(LOGGER_NAME);
    assert(log_);
    assert(gkfs::path::is_absolute(root_path_));
    // Verify that we have sufficient write access
    // This will throw on error, canceling daemon initialization
    auto test_file_path = "/.__chunk_dir_test"s;
    init_chunk_space(test_file_path);
    destroy_chunk_space(test_file_path);
    log_->debug("{}() Chunk storage initialized with path: '{}'", __func__, root_path_);
}

/**
 * Removes chunk directory with all its files
 * @param file_path
 * @throws ChunkStorageException
 */
void ChunkStorage::destroy_chunk_space(const string& file_path) const {
    auto chunk_dir = absolute(get_chunks_dir(file_path));
    try {
        // Note: remove_all does not throw an error when path doesn't exist.
        auto n = bfs::remove_all(chunk_dir);
        log_->debug("{}() Removed '{}' files from '{}'", __func__, n, chunk_dir);
    } catch (const bfs::filesystem_error& e) {
        auto err_str = fmt::format("{}() Failed to remove chunk directory. Path: '{}', Error: '{}'", __func__,
                                   chunk_dir, e.what());
        throw ChunkStorageException(e.code().value(), err_str);
    }
}

/**
 * Writes a chunk file.
 * On failure returns a negative error code corresponding to `-errno`.
 *
 * Refer to https://www.gnu.org/software/libc/manual/html_node/I_002fO-Primitives.html for pwrite behavior
 *
 * @param file_path
 * @param chunk_id
 * @param buff
 * @param size
 * @param offset
 * @param eventual
 * @throws ChunkStorageException (caller will handle eventual signalling)
 */
ssize_t
ChunkStorage::write_chunk(const string& file_path, gkfs::rpc::chnk_id_t chunk_id, const char* buff, size_t size,
                          off64_t offset) const {

    assert((offset + size) <= chunksize_);
    // may throw ChunkStorageException on failure
    init_chunk_space(file_path);

    auto chunk_path = absolute(get_chunk_path(file_path, chunk_id));
    int fd = open(chunk_path.c_str(), O_WRONLY | O_CREAT, 0640);
    if (fd < 0) {
        auto err_str = fmt::format("Failed to open chunk file for write. File: '{}', Error: '{}'", chunk_path,
                                   ::strerror(errno));
        throw ChunkStorageException(errno, err_str);
    }

    auto wrote = pwrite(fd, buff, size, offset);
    if (wrote < 0) {
        auto err_str = fmt::format("Failed to write chunk file. File: '{}', size: '{}', offset: '{}', Error: '{}'",
                                   chunk_path, size, offset, ::strerror(errno));
        throw ChunkStorageException(errno, err_str);
    }

    // if close fails we just write an entry into the log erroring out
    if (close(fd) < 0) {
        log_->warn("Failed to close chunk file after write. File: '{}', Error: '{}'",
                   chunk_path, ::strerror(errno));
    }
    return wrote;
}

/**
 * Read from a chunk file.
 * On failure returns a negative error code corresponding to `-errno`.
 *
 * Refer to https://www.gnu.org/software/libc/manual/html_node/I_002fO-Primitives.html for pread behavior
 * @param file_path
 * @param chunk_id
 * @param buf
 * @param size
 * @param offset
 * @param eventual
 */
ssize_t
ChunkStorage::read_chunk(const string& file_path, gkfs::rpc::chnk_id_t chunk_id, char* buf, size_t size,
                         off64_t offset) const {
    assert((offset + size) <= chunksize_);
    auto chunk_path = absolute(get_chunk_path(file_path, chunk_id));
    int fd = open(chunk_path.c_str(), O_RDONLY);
    if (fd < 0) {
        auto err_str = fmt::format("Failed to open chunk file for read. File: '{}', Error: '{}'", chunk_path,
                                   ::strerror(errno));
        throw ChunkStorageException(errno, err_str);
    }
    size_t tot_read = 0;
    ssize_t read = 0;

    do {
        read = pread64(fd,
                       buf + tot_read,
                       size - tot_read,
                       offset + tot_read);
        if (read == 0) {
            /*
             * A value of zero indicates end-of-file (except if the value of the size argument is also zero).
             * This is not considered an error. If you keep calling read while at end-of-file,
             * it will keep returning zero and doing nothing else.
             * Hence, we break here.
             */
            break;
        }

        if (read < 0) {
            auto err_str = fmt::format("Failed to read chunk file. File: '{}', size: '{}', offset: '{}', Error: '{}'",
                                       chunk_path, size, offset, ::strerror(errno));
            throw ChunkStorageException(errno, err_str);
        }

#ifndef NDEBUG
        if (tot_read + read < size) {
            log_->debug("Read less bytes than requested: '{}'/{}. Total read was '{}'. This is not an error!", read,
                        size - tot_read, size);
        }
#endif
        assert(read > 0);
        tot_read += read;
    } while (tot_read != size);


    // if close fails we just write an entry into the log erroring out
    if (close(fd) < 0) {
        log_->warn("Failed to close chunk file after read. File: '{}', Error: '{}'",
                   chunk_path, ::strerror(errno));
    }
    return tot_read;
}

/**
 * Delete all chunks starting with chunk a chunk id.
 * Note eventual consistency here: While chunks are removed, there is no lock that prevents
 * other processes from modifying anything in that directory.
 * It is the application's responsibility to stop modifying the file while truncate is executed
 *
 * If an error is encountered when removing a chunk file, the function will still remove all files and
 * report the error afterwards with ChunkStorageException.
 *
 * @param file_path
 * @param chunk_start
 * @param chunk_end
 * @throws ChunkStorageException
 */
void ChunkStorage::trim_chunk_space(const string& file_path, gkfs::rpc::chnk_id_t chunk_start) {

    auto chunk_dir = absolute(get_chunks_dir(file_path));
    const bfs::directory_iterator end;
    auto err_flag = false;
    for (bfs::directory_iterator chunk_file(chunk_dir); chunk_file != end; ++chunk_file) {
        auto chunk_path = chunk_file->path();
        auto chunk_id = ::stoul(chunk_path.filename().c_str());
        if (chunk_id >= chunk_start) {
            auto err = unlink(chunk_path.c_str());
            if (err == -1 && errno != ENOENT) {
                err_flag = true;
                log_->warn("{}() Failed to remove chunk file. File: '{}', Error: '{}'", __func__, chunk_path.native(),
                           ::strerror(errno));
            }
        }
    }
    if (err_flag)
        throw ChunkStorageException(EIO, fmt::format("{}() One or more errors occurred when truncating '{}'", __func__,
                                                     file_path));
}

/**
 * Truncates a single chunk file to a given length
 * @param file_path
 * @param chunk_id
 * @param length
 * @throws ChunkStorageException
 */
void ChunkStorage::truncate_chunk_file(const string& file_path, gkfs::rpc::chnk_id_t chunk_id, off_t length) {
    auto chunk_path = absolute(get_chunk_path(file_path, chunk_id));
    assert(length > 0 && static_cast<gkfs::rpc::chnk_id_t>(length) <= chunksize_);
    int ret = truncate64(chunk_path.c_str(), length);
    if (ret == -1) {
        auto err_str = fmt::format("Failed to truncate chunk file. File: '{}', Error: '{}'", chunk_path,
                                   ::strerror(errno));
        throw ChunkStorageException(errno, err_str);
    }
}

/**
 * Calls statfs on the chunk directory to get statistic on its used and free size left
 * @return ChunkStat
 * @throws ChunkStorageException
 */
ChunkStat ChunkStorage::chunk_stat() const {
    struct statfs sfs{};

    if (statfs(root_path_.c_str(), &sfs) != 0) {
        auto err_str = fmt::format("Failed to get filesystem statistic for chunk directory. Error: '{}'",
                                   ::strerror(errno));
        throw ChunkStorageException(errno, err_str);
    }

    log_->debug("Chunksize '{}', total '{}', free '{}'", sfs.f_bsize, sfs.f_blocks, sfs.f_bavail);
    auto bytes_total =
            static_cast<unsigned long long>(sfs.f_bsize) *
            static_cast<unsigned long long>(sfs.f_blocks);
    auto bytes_free =
            static_cast<unsigned long long>(sfs.f_bsize) *
            static_cast<unsigned long long>(sfs.f_bavail);
    return {chunksize_,
            bytes_total / chunksize_,
            bytes_free / chunksize_};
}

} // namespace data
} // namespace gkfs