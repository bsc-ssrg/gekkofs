#include <sys/statfs.h>

#include <global/configure.hpp>
#include <global/path_util.hpp>
#include <preload/preload.hpp>
#include <preload/adafs_functions.hpp>
#include <preload/rpc/ld_rpc_metadentry.hpp>
#include <preload/rpc/ld_rpc_data_ws.hpp>
#include <preload/open_dir.hpp>

using namespace std;

int adafs_open(const std::string& path, mode_t mode, int flags) {
    int err = 0;

    if(flags & O_PATH){
        CTX->log()->error("{}() `O_PATH` flag is not supported", __func__);
        errno = ENOTSUP;
        return -1;
    }

    if(flags & O_APPEND){
        CTX->log()->error("{}() `O_APPEND` flag is not supported", __func__);
        errno = ENOTSUP;
        return -1;
    }

    bool exists = true;
    struct stat st;
    err = adafs_stat(path, &st);
    if(err) {
        if(errno == ENOENT) {
            exists = false;
        } else {
            CTX->log()->error("{}() error while retriving stat to file", __func__);
            return -1;
        }
    }

    if (!exists) {
        if (! (flags & O_CREAT)) {
            // file doesn't exists and O_CREAT was not set
            errno = ENOENT;
            return -1;
        }

        /***   CREATION    ***/
        assert(flags & O_CREAT);

        if(flags & O_DIRECTORY){
            CTX->log()->error("{}() O_DIRECTORY use with O_CREAT. NOT SUPPORTED", __func__);
            errno = ENOTSUP;
            return -1;
        }

        // no access check required here. If one is using our FS they have the permissions.
        if(adafs_mk_node(path, mode | S_IFREG)) {
            CTX->log()->error("{}() error creating non-existent file", __func__);
            return -1;
        }
    } else {
        /* File already exists */

        if(flags & O_EXCL) {
            // File exists and O_EXCL was set
            errno = EEXIST;
            return -1;
        }

#if defined(CHECK_ACCESS_DURING_OPEN)
        auto mask = F_OK; // F_OK == 0
        if ((mode & S_IRUSR) || (mode & S_IRGRP) || (mode & S_IROTH))
            mask = mask & R_OK;
        if ((mode & S_IWUSR) || (mode & S_IWGRP) || (mode & S_IWOTH))
            mask = mask & W_OK;
        if ((mode & S_IXUSR) || (mode & S_IXGRP) || (mode & S_IXOTH))
            mask = mask & X_OK;

        if( ! ((mask & md.mode()) == mask)) {
            errno = EACCES;
            return -1;
        }
#endif

        if(S_ISDIR(st.st_mode)) {
            return adafs_opendir(path);
        }

        /*** Regular file exists ***/
        assert(S_ISREG(st.st_mode));

        if( (flags & O_TRUNC) && ((flags & O_RDWR) || (flags & O_WRONLY)) ) {
            if(adafs_truncate(path, st.st_size, 0)) {
                CTX->log()->error("{}() error truncating file", __func__);
                return -1;
            }
        }
    }

    return CTX->file_map()->add(std::make_shared<OpenFile>(path, flags));
}

int adafs_mk_node(const std::string& path, const mode_t mode) {

    //file type must be set
    assert((mode & S_IFMT) != 0);
    //file type must be either regular file or directory
    assert(S_ISREG(mode) || S_ISDIR(mode));

    struct stat st;
    auto p_comp = dirname(path);
    auto ret = adafs_stat(p_comp, &st);
    if(ret != 0) {
        CTX->log()->debug("{}() parent component does not exists: '{}'", __func__, p_comp);
        errno = ENOENT;
        return -1;
    }
    if(!S_ISDIR(st.st_mode)) {
        CTX->log()->debug("{}() parent component is not a direcotory: '{}'", __func__, p_comp);
        errno = ENOTDIR;
        return -1;
    }
    return rpc_send_mk_node(path, mode);
}

/**
 * This sends internally a broadcast (i.e. n RPCs) to clean their chunk folders for that path
 * @param path
 * @return
 */
int adafs_rm_node(const std::string& path) {
    struct stat node_metadentry{};
    auto err = adafs_stat(path, &node_metadentry);
    if (err != 0)
        return -1;
    return rpc_send_rm_node(path, node_metadentry.st_size == 0);
}

int adafs_access(const std::string& path, const int mask) {
#if !defined(DO_LOOKUP)
    // object is assumed to be existing, even though it might not
    return 0;
#endif
#if defined(CHECK_ACCESS)
    return rpc_send_access(path, mask);
#else
    return rpc_send_access(path, F_OK); // Only check for file exists
#endif
}

int adafs_stat(const string& path, struct stat* buf) {
    std::string attr;
    auto err = rpc_send_stat(path, attr);
    if (err) {
        return err;
    }
    Metadata md(attr);
    metadata_to_stat(path, md, *buf);
    return err;
}

int adafs_statfs(const string& path, struct statfs* adafs_buf, struct statfs& realfs_buf) {
    // Check that file path exists
    auto ret = rpc_send_access(path, F_OK);
    // Valid fs error
    if (ret > 0) {
        errno = ret;
        return -1;
    }
    // RPC error (errno has been set)
    if (ret < 0)
        return -1;

    // fs object exists. Let's make up some fs values
    adafs_buf->f_type = 0; // fs is not know to VFS. Therefore, no valid specifier possible
    adafs_buf->f_bsize = static_cast<int>(CHUNKSIZE);
    // some rough estimations
    adafs_buf->f_blocks = realfs_buf.f_blocks * CTX->fs_conf()->host_size;
    adafs_buf->f_bfree = realfs_buf.f_bfree * CTX->fs_conf()->host_size;
    adafs_buf->f_bavail = realfs_buf.f_bavail * CTX->fs_conf()->host_size;
    adafs_buf->f_files = realfs_buf.f_files * CTX->fs_conf()->host_size;
    adafs_buf->f_ffree = realfs_buf.f_ffree * CTX->fs_conf()->host_size;
    adafs_buf->f_fsid = realfs_buf.f_fsid; // "Nobody knows what f_fsid is supposed to contain"
    adafs_buf->f_namelen = realfs_buf.f_namelen;
    adafs_buf->f_frsize = realfs_buf.f_frsize;
    adafs_buf->f_spare[0] = 0;
    adafs_buf->f_spare[1] = 0;
    adafs_buf->f_spare[2] = 0;
    adafs_buf->f_spare[3] = 0;
    adafs_buf->f_flags = ST_NOATIME | ST_NOSUID | ST_NODEV | ST_SYNCHRONOUS;
    if (!CTX->fs_conf()->atime_state)
        adafs_buf->f_flags = adafs_buf->f_flags | ST_NOATIME | ST_NODIRATIME;
    return 0;
}

off64_t adafs_lseek(int fd, off64_t offset, int whence) {
    return adafs_lseek(CTX->file_map()->get(fd), offset, whence);
}

off64_t adafs_lseek(shared_ptr<OpenFile> adafs_fd, off64_t offset, int whence) {
    switch (whence) {
        case SEEK_SET:
            adafs_fd->pos(offset);
            break;
        case SEEK_CUR:
            adafs_fd->pos(adafs_fd->pos() + offset);
            break;
        case SEEK_END: {
            off64_t file_size;
            auto err = rpc_send_get_metadentry_size(adafs_fd->path(), file_size);
            if (err < 0) {
                errno = err; // Negative numbers are explicitly for error codes
                return -1;
            }
            adafs_fd->pos(file_size + offset);
            break;
        }
        case SEEK_DATA:
            // We do not support this whence yet
            errno = EINVAL;
            return -1;
        case SEEK_HOLE:
            // We do not support this whence yet
            errno = EINVAL;
            return -1;
        default:
            errno = EINVAL;
            return -1;
    }
    return adafs_fd->pos();
}

int adafs_truncate(const std::string& path, off_t old_size, off_t new_size) {
    assert(new_size >= 0);
    assert(new_size <= old_size);

    if(new_size == old_size) {
        return 0;
    }

    if (rpc_send_decr_size(path, new_size)) {
        CTX->log()->debug("{}() failed to decrease size", __func__);
        return -1;
    }

    if(rpc_send_trunc_data(path, old_size, new_size)){
        CTX->log()->debug("{}() failed to truncate data", __func__);
        return -1;
    }
    return 0;
}

int adafs_truncate(const std::string& path, off_t length) {
    /* TODO CONCURRENCY:
     * At the moment we first ask the length to the metadata-server in order to
     * know which data-server have data to be deleted.
     *
     * From the moment we issue the adafs_stat and the moment we issue the
     * adafs_trunc_data, some more data could have been added to the file and the
     * length increased.
     */
    if(length < 0) {
        CTX->log()->debug("{}() length is negative: {}", __func__, length);
        errno = EINVAL;
        return -1;
    }

    struct stat st;
    if(adafs_stat(path, &st)) {
        return -1;
    }

    if(length > st.st_size) {
        CTX->log()->debug("{}() length is greater then file size: {} > {}",
               __func__, length, st.st_size);
        errno = EINVAL;
        return -1;
    }
    return adafs_truncate(path, st.st_size, length);
}

int adafs_dup(const int oldfd) {
    return CTX->file_map()->dup(oldfd);
}

int adafs_dup2(const int oldfd, const int newfd) {
    return CTX->file_map()->dup2(oldfd, newfd);
}

ssize_t adafs_write(int fd, void* buf, size_t count) {
            auto adafs_fd = CTX->file_map()->get(fd);
            auto pos = adafs_fd->pos(); //retrieve the current offset
            if (adafs_fd->get_flag(OpenFile_flags::append))
                adafs_lseek(adafs_fd, 0, SEEK_END);
            auto ret = adafs_pwrite_ws(fd, buf, count, pos);
            // Update offset in file descriptor in the file map
            if (ret > 0) {
                adafs_fd->pos(pos + count);
            }
            return ret;
}

ssize_t adafs_pwrite_ws(int fd, const void* buf, size_t count, off64_t offset) {
    auto adafs_fd = CTX->file_map()->get(fd);
    if (adafs_fd->type() != FileType::regular) {
        assert(adafs_fd->type() == FileType::directory);
        CTX->log()->warn("{}() cannot read from directory", __func__);
        errno = EISDIR;
        return -1;
    }
    auto path = make_shared<string>(adafs_fd->path());
    CTX->log()->trace("{}() fd: {}, count: {}, offset: {}", __func__, fd, count, offset);
    auto append_flag = adafs_fd->get_flag(OpenFile_flags::append);
    ssize_t ret = 0;
    long updated_size = 0;

    ret = rpc_send_update_metadentry_size(*path, count, offset, append_flag, updated_size);
    if (ret != 0) {
        CTX->log()->error("{}() update_metadentry_size failed with ret {}", __func__, ret);
        return 0; // ERR
    }
    ret = rpc_send_write(*path, buf, append_flag, offset, count, updated_size);
    if (ret < 0) {
        CTX->log()->warn("{}() rpc_send_write failed with ret {}", __func__, ret);
    }
    return ret; // return written size or -1 as error
}

ssize_t adafs_read(int fd, void* buf, size_t count) {
            auto adafs_fd = CTX->file_map()->get(fd);
            auto pos = adafs_fd->pos(); //retrieve the current offset
            auto ret = adafs_pread_ws(fd, buf, count, pos);
            // Update offset in file descriptor in the file map
            if (ret > 0) {
                adafs_fd->pos(pos + ret);
            }
            return ret;
}

ssize_t adafs_pread_ws(int fd, void* buf, size_t count, off64_t offset) {
    auto adafs_fd = CTX->file_map()->get(fd);
    if (adafs_fd->type() != FileType::regular) {
        assert(adafs_fd->type() == FileType::directory);
        CTX->log()->warn("{}() cannot read from directory", __func__);
        errno = EISDIR;
        return -1;
    }
    auto path = make_shared<string>(adafs_fd->path());
    CTX->log()->trace("{}() fd: {}, count: {}, offset: {}", __func__, fd, count, offset);
    // Zeroing buffer before read is only relevant for sparse files. Otherwise sparse regions contain invalid data.
#if defined(ZERO_BUFFER_BEFORE_READ)
    memset(buf, 0, sizeof(char)*count);
#endif
    auto ret = rpc_send_read(*path, buf, offset, count);
    if (ret < 0) {
        CTX->log()->warn("{}() rpc_send_read failed with ret {}", __func__, ret);
    }
    // XXX check that we don't try to read past end of the file
    return ret; // return read size or -1 as error
}

int adafs_opendir(const std::string& path) {

    struct stat st;
    if(adafs_stat(path, &st)) {
        return -1;
    }
    if(!(st.st_mode & S_IFDIR)) {
        CTX->log()->debug("{}() path is not a directory", __func__);
        errno = ENOTDIR;
        return -1;
    }

    auto open_dir = std::make_shared<OpenDir>(path);
    rpc_send_get_dirents(*open_dir);
    return CTX->file_map()->add(open_dir);
}

int adafs_rmdir(const std::string& path) {
#if defined(DO_LOOKUP)
    auto err = rpc_send_access(path, F_OK);
    if(err != 0){
        return err;
    }
#endif
    auto open_dir = std::make_shared<OpenDir>(path);
    rpc_send_get_dirents(*open_dir);
    if(open_dir->size() != 0){
        errno = ENOTEMPTY;
        return -1;
    }
    return rpc_send_rm_node(path, true);
}

struct dirent * adafs_readdir(int fd){
    CTX->log()->trace("{}() called on fd: {}", __func__, fd);
    auto open_file = CTX->file_map()->get(fd);
    assert(open_file != nullptr);
    auto open_dir = static_pointer_cast<OpenDir>(open_file);
    if(!open_dir){
        //Cast did not succeeded: open_file is a regular file
        errno = EBADF;
        return nullptr;
    }
    return open_dir->readdir();
}