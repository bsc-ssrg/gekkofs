
#include <daemon/adafs_ops/data.hpp>

using namespace std;

std::string path_to_fspath(const std::string& path) {
    // root path is absolute as is the path comes in here which is hierarchically under root_path
    // XXX check if this can be done easier
    string fs_path;
    set_difference(path.begin(), path.end(), ADAFS_DATA->mountdir().begin(), ADAFS_DATA->mountdir().end(),
                   std::back_inserter(fs_path));
    if (fs_path.at(0) == '/') {
        fs_path = fs_path.substr(1, fs_path.size());
    }
    // replace / with : to avoid making a bunch of mkdirs to store the data in the underlying fs. XXX Can this be done with hashing?
    replace(fs_path.begin(), fs_path.end(), '/', ':');
    return fs_path;
}

/**
 * Remove the directory in the chunk dir of a file.
 * @param inode
 * @return
 */
int destroy_chunk_space(const std::string& path) {
    auto fs_path = path_to_fspath(path);
    auto chnk_path = bfs::path(ADAFS_DATA->chunk_path());
    chnk_path /= fs_path;

    // remove chunk dir with all contents if path exists
    try {
        bfs::remove_all(chnk_path);
    } catch (const bfs::filesystem_error& e){
        ADAFS_DATA->spdlogger()->error("{}() Recursive remove failed; {}", __func__, e.what());
        return -1;
    }
    return 0;
}

/**
 * Used by an argobots threads. Argument args has the following fields:
 * const std::string* path;
   const char* buf;
   const rpc_chnk_id_t* chnk_id;
   size_t size;
   off64_t off;
   ABT_eventual* eventual;
 * This function is driven by the IO pool. so there is a maximum allowed number of concurrent IO operations per daemon.
 * This function is called by tasklets, as this function cannot be allowed to block.
 * @return written_size<size_t> is put into eventual and returned that way
 */
void write_file_abt(void* _arg) {
    size_t write_size = 0;
    // Unpack args
    auto* arg = static_cast<struct write_chunk_args*>(_arg);
    auto fs_path = path_to_fspath(*arg->path);
    auto chnk_path = bfs::path(ADAFS_DATA->chunk_path());
    chnk_path /= fs_path;
    bfs::create_directories(chnk_path);
    chnk_path /= fmt::FormatInt(arg->chnk_id).c_str();
    // open file
    int fd = open(chnk_path.c_str(), O_WRONLY | O_CREAT, 0640);
    if (fd < 0) {
        write_size = static_cast<size_t>(EIO);
        ABT_eventual_set(arg->eventual, &write_size, sizeof(size_t));
        return;
    }
    // write file
    auto err = pwrite64(fd, arg->buf, arg->size, arg->off);
    if (err < 0) {
        ADAFS_DATA->spdlogger()->error("{}() Error {} while pwriting file {} chunk_id {} size {} off {}", __func__,
                                       strerror(errno), chnk_path.c_str(), arg->chnk_id, arg->size, arg->off);
    } else {
        write_size = static_cast<size_t>(err); // This is cast safe
    }
    ABT_eventual_set(arg->eventual, &write_size, sizeof(size_t));
    // file is closed
    close(fd);
}

/**
 * Used by an argobots threads. Argument args has the following fields:
 * const std::string* path;
   char* buf;
   const rpc_chnk_id_t* chnk_id;
   size_t size;
   off64_t off;
   ABT_eventual* eventual;
 * This function is driven by the IO pool. so there is a maximum allowed number of concurrent IO operations per daemon.
 * This function is called by tasklets, as this function cannot be allowed to block.
 * @return read_size<size_t> is put into eventual and returned that way
 */
void read_file_abt(void* _arg) {
    size_t read_size = 0;
    //unpack args
    auto* arg = static_cast<struct read_chunk_args*>(_arg);
    auto fs_path = path_to_fspath(*arg->path);
    auto chnk_path = bfs::path(ADAFS_DATA->chunk_path());
    chnk_path /= fs_path;
    chnk_path /= fmt::FormatInt(arg->chnk_id).c_str();;

    int fd = open(chnk_path.c_str(), R_OK);
    if (fd < 0) { //the case that file does not exist e.g sparse file or random write
        read_size = static_cast<size_t>(0); 
        ABT_eventual_set(arg->eventual, &read_size, sizeof(size_t));
        return;
    }
    auto err = pread64(fd, arg->buf, arg->size, arg->off);
    if (err < 0) {
        ADAFS_DATA->spdlogger()->error("{}() Error {} while preading file {} chunk_id {} size {} off {}", __func__,
                                       strerror(errno), chnk_path.c_str(), arg->chnk_id, arg->size, arg->off);
    } else {
        read_size = static_cast<size_t>(err); // This is cast safe
    }
    close(fd);
    ABT_eventual_set(arg->eventual, &read_size, sizeof(size_t));
}