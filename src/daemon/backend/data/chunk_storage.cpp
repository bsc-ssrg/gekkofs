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
#include <boost/filesystem.hpp>
#include <spdlog/spdlog.h>

extern "C" {
#include <sys/statfs.h>
}

namespace bfs = boost::filesystem;
using namespace std;

namespace gkfs {
namespace data {

string ChunkStorage::absolute(const string& internal_path) const {
    assert(gkfs::path::is_relative(internal_path));
    return root_path + '/' + internal_path;
}

ChunkStorage::ChunkStorage(const string& path, const size_t chunksize) :
        root_path(path),
        chunksize(chunksize) {
    //TODO check path: absolute, exists, permission to write etc...
    assert(gkfs::path::is_absolute(root_path));

    /* Initialize logger */
    log = spdlog::get(LOGGER_NAME);
    assert(log);

    log->debug("Chunk storage initialized with path: '{}'", root_path);
}

string ChunkStorage::get_chunks_dir(const string& file_path) {
    assert(gkfs::path::is_absolute(file_path));
    string chunk_dir = file_path.substr(1);
    ::replace(chunk_dir.begin(), chunk_dir.end(), '/', ':');
    return chunk_dir;
}

string ChunkStorage::get_chunk_path(const string& file_path, unsigned int chunk_id) {
    return get_chunks_dir(file_path) + '/' + ::to_string(chunk_id);
}

void ChunkStorage::destroy_chunk_space(const string& file_path) const {
    auto chunk_dir = absolute(get_chunks_dir(file_path));
    try {
        bfs::remove_all(chunk_dir);
    } catch (const bfs::filesystem_error& e) {
        log->error("Failed to remove chunk directory. Path: '{}', Error: '{}'", chunk_dir, e.what());
    }
}

void ChunkStorage::init_chunk_space(const string& file_path) const {
    auto chunk_dir = absolute(get_chunks_dir(file_path));
    auto err = mkdir(chunk_dir.c_str(), 0750);
    if (err == -1 && errno != EEXIST) {
        log->error("Failed to create chunk dir. Path: '{}', Error: '{}'", chunk_dir, ::strerror(errno));
        throw ::system_error(errno, ::system_category(), "Failed to create chunk directory");
    }
}

/* Delete all chunks stored on this node that falls in the gap [chunk_start, chunk_end]
 *
 * This is pretty slow method because it cycle over all the chunks sapce for this file.
 */
void ChunkStorage::trim_chunk_space(const string& file_path,
                                    unsigned int chunk_start, unsigned int chunk_end) {

    auto chunk_dir = absolute(get_chunks_dir(file_path));
    const bfs::directory_iterator end;

    for (bfs::directory_iterator chunk_file(chunk_dir); chunk_file != end; ++chunk_file) {
        auto chunk_path = chunk_file->path();
        auto chunk_id = ::stoul(chunk_path.filename().c_str());
        if (chunk_id >= chunk_start && chunk_id <= chunk_end) {
            int ret = unlink(chunk_path.c_str());
            if (ret == -1) {
                log->error("Failed to remove chunk file. File: '{}', Error: '{}'", chunk_path.native(),
                           ::strerror(errno));
                throw ::system_error(errno, ::system_category(), "Failed to remove chunk file");
            }
        }
    }
}

void ChunkStorage::delete_chunk(const string& file_path, unsigned int chunk_id) {
    auto chunk_path = absolute(get_chunk_path(file_path, chunk_id));
    int ret = unlink(chunk_path.c_str());
    if (ret == -1) {
        log->error("Failed to remove chunk file. File: '{}', Error: '{}'", chunk_path, ::strerror(errno));
        throw ::system_error(errno, ::system_category(), "Failed to remove chunk file");
    }
}

void ChunkStorage::truncate_chunk(const string& file_path, unsigned int chunk_id, off_t length) {
    auto chunk_path = absolute(get_chunk_path(file_path, chunk_id));
    assert(length > 0 && (unsigned int) length <= chunksize);
    int ret = truncate(chunk_path.c_str(), length);
    if (ret == -1) {
        log->error("Failed to truncate chunk file. File: '{}', Error: '{}'", chunk_path, ::strerror(errno));
        throw ::system_error(errno, ::system_category(), "Failed to truncate chunk file");
    }
}

void ChunkStorage::write_chunk(const string& file_path, unsigned int chunk_id,
                               const char* buff, size_t size, off64_t offset, ABT_eventual& eventual) const {

    assert((offset + size) <= chunksize);

    init_chunk_space(file_path);

    auto chunk_path = absolute(get_chunk_path(file_path, chunk_id));
    int fd = open(chunk_path.c_str(), O_WRONLY | O_CREAT, 0640);
    if (fd < 0) {
        log->error("Failed to open chunk file for write. File: '{}', Error: '{}'", chunk_path, ::strerror(errno));
        throw ::system_error(errno, ::system_category(), "Failed to open chunk file for write");
    }

    auto wrote = pwrite(fd, buff, size, offset);
    if (wrote < 0) {
        log->error("Failed to write chunk file. File: '{}', size: '{}', offset: '{}', Error: '{}'",
                   chunk_path, size, offset, ::strerror(errno));
        throw ::system_error(errno, ::system_category(), "Failed to write chunk file");
    }

    ABT_eventual_set(eventual, &wrote, sizeof(size_t));

    auto err = close(fd);
    if (err < 0) {
        log->error("Failed to close chunk file after write. File: '{}', Error: '{}'",
                   chunk_path, ::strerror(errno));
        //throw ::system_error(errno, ::system_category(), "Failed to close chunk file");
    }
}

void ChunkStorage::read_chunk(const string& file_path, unsigned int chunk_id,
                              char* buff, size_t size, off64_t offset, ABT_eventual& eventual) const {
    assert((offset + size) <= chunksize);
    auto chunk_path = absolute(get_chunk_path(file_path, chunk_id));
    int fd = open(chunk_path.c_str(), O_RDONLY);
    if (fd < 0) {
        log->error("Failed to open chunk file for read. File: '{}', Error: '{}'", chunk_path, ::strerror(errno));
        throw ::system_error(errno, ::system_category(), "Failed to open chunk file for read");
    }
    size_t tot_read = 0;
    ssize_t read = 0;

    do {
        read = pread64(fd,
                       buff + tot_read,
                       size - tot_read,
                       offset + tot_read);
        if (read == 0) {
            break;
        }

        if (read < 0) {
            log->error("Failed to read chunk file. File: '{}', size: '{}', offset: '{}', Error: '{}'",
                       chunk_path, size, offset, ::strerror(errno));
            throw ::system_error(errno, ::system_category(), "Failed to read chunk file");
        }

#ifndef NDEBUG
        if (tot_read + read < size) {
            log->warn("Read less bytes than requested: '{}'/{}. Total read was '{}'", read, size - tot_read, size);
        }
#endif
        assert(read > 0);
        tot_read += read;


    } while (tot_read != size);

    ABT_eventual_set(eventual, &tot_read, sizeof(size_t));

    auto err = close(fd);
    if (err < 0) {
        log->error("Failed to close chunk file after read. File: '{}', Error: '{}'",
                   chunk_path, ::strerror(errno));
        //throw ::system_error(errno, ::system_category(), "Failed to close chunk file");
    }
}

ChunkStat ChunkStorage::chunk_stat() const {
    struct statfs sfs{};
    if (statfs(root_path.c_str(), &sfs) != 0) {
        log->error("Failed to get filesystem statistic for chunk directory."
                   " Error: '{}'", ::strerror(errno));
        throw ::system_error(errno, ::system_category(),
                             "statfs() failed on chunk directory");
    }

    log->debug("Chunksize '{}', total '{}', free '{}'", sfs.f_bsize, sfs.f_blocks, sfs.f_bavail);
    auto bytes_total =
            static_cast<unsigned long long>(sfs.f_bsize) *
            static_cast<unsigned long long>(sfs.f_blocks);
    auto bytes_free =
            static_cast<unsigned long long>(sfs.f_bsize) *
            static_cast<unsigned long long>(sfs.f_bavail);
    return {chunksize,
            bytes_total / chunksize,
            bytes_free / chunksize};
}

} // namespace data
} // namespace gkfs