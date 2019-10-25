/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <sys/statfs.h>
#include <sys/statvfs.h>

#include <global/configure.hpp>
#include <global/path_util.hpp>
#include <client/preload.hpp>
#include "client/preload_util.hpp"
#include <client/adafs_functions.hpp>
#include <client/rpc/ld_rpc_metadentry.hpp>
#include <client/rpc/ld_rpc_data_ws.hpp>
#include <client/open_dir.hpp>


#define __ALIGN_KERNEL_MASK(x, mask)	(((x) + (mask)) & ~(mask))
#define __ALIGN_KERNEL(x, a)		    __ALIGN_KERNEL_MASK(x, (typeof(x))(a) - 1)
#define ALIGN(x, a)                     __ALIGN_KERNEL((x), (a))

struct linux_dirent {
    unsigned long	d_ino;
    unsigned long	d_off;
    unsigned short	d_reclen;
    char		d_name[1];
};

struct linux_dirent64 {
    unsigned long long  d_ino;
    unsigned long long  d_off;
    unsigned short  d_reclen;
    unsigned char d_type;
    char        d_name[1];
};

using namespace std;

int adafs_open(const std::string& path, mode_t mode, int flags) {

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
    auto md = adafs_metadata(path);
    if (!md) {
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

#ifdef HAS_SYMLINKS
        if (md->is_link()) {
            if (flags & O_NOFOLLOW) {
                CTX->log()->warn("{}() symlink found and O_NOFOLLOW flag was specified", __func__);
                errno = ELOOP;
                return -1;
            }
            return adafs_open(md->target_path(), mode, flags);
        }
#endif

        if(S_ISDIR(md->mode())) {
            return adafs_opendir(path);
        }


        /*** Regular file exists ***/
        assert(S_ISREG(md->mode()));

        if( (flags & O_TRUNC) && ((flags & O_RDWR) || (flags & O_WRONLY)) ) {
            if(adafs_truncate(path, md->size(), 0)) {
                CTX->log()->error("{}() error truncating file", __func__);
                return -1;
            }
        }
    }

    return CTX->file_map()->add(std::make_shared<OpenFile>(path, flags));
}

int adafs_mk_node(const std::string& path, mode_t mode) {

    //file type must be set
    switch (mode & S_IFMT) {
        case 0:
            mode |= S_IFREG;
            break;
        case S_IFREG: // intentionally fall-through
        case S_IFDIR:
            break;
        case S_IFCHR: // intentionally fall-through
        case S_IFBLK:
        case S_IFIFO:
        case S_IFSOCK:
            CTX->log()->warn("{}() unsupported node type", __func__);
            errno = ENOTSUP;
            return -1;
        default:
            CTX->log()->warn("{}() unrecognized node type", __func__);
            errno = EINVAL;
            return -1;
    }

    auto p_comp = dirname(path);
    auto md = adafs_metadata(p_comp);
    if (!md) {
        CTX->log()->debug("{}() parent component does not exists: '{}'", __func__, p_comp);
        errno = ENOENT;
        return -1;
    }
    if (!S_ISDIR(md->mode())) {
        CTX->log()->debug("{}() parent component is not a direcotory: '{}'", __func__, p_comp);
        errno = ENOTDIR;
        return -1;
    }
    return rpc_send::mk_node(path, mode);
}

/**
 * This sends internally a broadcast (i.e. n RPCs) to clean their chunk folders for that path
 * @param path
 * @return
 */
int adafs_rm_node(const std::string& path) {
    auto md = adafs_metadata(path);
    if (!md) {
        return -1;
    }
    bool has_data = S_ISREG(md->mode()) && (md->size() != 0);
    return rpc_send::rm_node(path, !has_data, md->size());
}

int adafs_access(const std::string& path, const int mask, bool follow_links) {
    auto md = adafs_metadata(path, follow_links);
    if (!md) {
        errno = ENOENT;
        return -1;
    }
    return 0;
}

int adafs_stat(const string& path, struct stat* buf, bool follow_links) {
    auto md = adafs_metadata(path, follow_links);
    if (!md) {
        return -1;
    }
    metadata_to_stat(path, *md, *buf);
    return 0;
}

std::shared_ptr<Metadata> adafs_metadata(const string& path, bool follow_links) {
    std::string attr;
    auto err = rpc_send::stat(path, attr);
    if (err) {
        return nullptr;
    }
#ifdef HAS_SYMLINKS
    if (follow_links) {
        Metadata md{attr};
        while (md.is_link()) {
            err = rpc_send::stat(md.target_path(), attr);
            if (err) {
                return nullptr;
            }
            md = Metadata{attr};
        }
    }
#endif
    return make_shared<Metadata>(attr);
}

int adafs_statfs(struct statfs* buf) {
    CTX->log()->trace("{}() called", __func__);
    auto blk_stat = rpc_send::chunk_stat();
    buf->f_type = 0;
    buf->f_bsize = blk_stat.chunk_size;
    buf->f_blocks = blk_stat.chunk_total;
    buf->f_bfree = blk_stat.chunk_free;
    buf->f_bavail = blk_stat.chunk_free;
    buf->f_files = 0;
    buf->f_ffree = 0;
    buf->f_fsid = {0, 0};
    buf->f_namelen = PATH_MAX_LEN;
    buf->f_frsize = 0;
    buf->f_flags =
        ST_NOATIME | ST_NODIRATIME | ST_NOSUID | ST_NODEV | ST_SYNCHRONOUS;
    return 0;
}

int adafs_statvfs(struct statvfs* buf) {
    init_ld_env_if_needed();
    auto blk_stat = rpc_send::chunk_stat();
    buf->f_bsize = blk_stat.chunk_size;
    buf->f_blocks = blk_stat.chunk_total;
    buf->f_bfree = blk_stat.chunk_free;
    buf->f_bavail = blk_stat.chunk_free;
    buf->f_files = 0;
    buf->f_ffree = 0;
    buf->f_favail = 0;
    buf->f_fsid = 0;
    buf->f_namemax = PATH_MAX_LEN;
    buf->f_frsize = 0;
    buf->f_flag =
        ST_NOATIME | ST_NODIRATIME | ST_NOSUID | ST_NODEV | ST_SYNCHRONOUS;
    return 0;
}

off_t adafs_lseek(unsigned int fd, off_t offset, unsigned int whence) {
    return adafs_lseek(CTX->file_map()->get(fd), offset, whence);
}

off_t adafs_lseek(shared_ptr<OpenFile> adafs_fd, off_t offset, unsigned int whence) {
    switch (whence) {
        case SEEK_SET:
            CTX->log()->debug("{}() whence is SEEK_SET", __func__);
            adafs_fd->pos(offset);
            break;
        case SEEK_CUR:
            CTX->log()->debug("{}() whence is SEEK_CUR", __func__);
            adafs_fd->pos(adafs_fd->pos() + offset);
            break;
        case SEEK_END: {
            CTX->log()->debug("{}() whence is SEEK_END", __func__);
            off64_t file_size;
            auto err = rpc_send::get_metadentry_size(adafs_fd->path(), file_size);
            if (err < 0) {
                errno = err; // Negative numbers are explicitly for error codes
                return -1;
            }
            adafs_fd->pos(file_size + offset);
            break;
        }
        case SEEK_DATA:
            CTX->log()->warn("{}() SEEK_DATA whence is not supported", __func__);
            // We do not support this whence yet
            errno = EINVAL;
            return -1;
        case SEEK_HOLE:
            CTX->log()->warn("{}() SEEK_HOLE whence is not supported", __func__);
            // We do not support this whence yet
            errno = EINVAL;
            return -1;
        default:
            CTX->log()->warn("{}() unknown whence {}", __func__, whence);
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

    if (rpc_send::decr_size(path, new_size)) {
        CTX->log()->debug("{}() failed to decrease size", __func__);
        return -1;
    }

    if(rpc_send::trunc_data(path, old_size, new_size)){
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

    auto md = adafs_metadata(path, true);
    if (!md) {
        return -1;
    }
    auto size = md->size();
    if(static_cast<unsigned long>(length) > size) {
        CTX->log()->debug("{}() length is greater then file size: {} > {}",
               __func__, length, size);
        errno = EINVAL;
        return -1;
    }
    return adafs_truncate(path, size, length);
}

int adafs_dup(const int oldfd) {
    return CTX->file_map()->dup(oldfd);
}

int adafs_dup2(const int oldfd, const int newfd) {
    return CTX->file_map()->dup2(oldfd, newfd);
}

ssize_t adafs_pwrite(std::shared_ptr<OpenFile> file, const char * buf, size_t count, off64_t offset) {
    if (file->type() != FileType::regular) {
        assert(file->type() == FileType::directory);
        CTX->log()->warn("{}() cannot read from directory", __func__);
        errno = EISDIR;
        return -1;
    }
    auto path = make_shared<string>(file->path());
    CTX->log()->trace("{}() count: {}, offset: {}", __func__, count, offset);
    auto append_flag = file->get_flag(OpenFile_flags::append);
    ssize_t ret = 0;
    long updated_size = 0;

    ret = rpc_send::update_metadentry_size(*path, count, offset, append_flag, updated_size);
    if (ret != 0) {
        CTX->log()->error("{}() update_metadentry_size failed with ret {}", __func__, ret);
        return ret; // ERR
    }
    ret = rpc_send::write(*path, buf, append_flag, offset, count, updated_size);
    if (ret < 0) {
        CTX->log()->warn("{}() rpc_send::write failed with ret {}", __func__, ret);
    }
    return ret; // return written size or -1 as error
}

ssize_t adafs_pwrite_ws(int fd, const void* buf, size_t count, off64_t offset) {
    auto file = CTX->file_map()->get(fd);
    return adafs_pwrite(file, reinterpret_cast<const char*>(buf), count, offset);
}

/* Write counts bytes starting from current file position
 * It also update the file position accordingly
 *
 * Same as write syscall.
*/
ssize_t adafs_write(int fd, const void * buf, size_t count) {
    auto adafs_fd = CTX->file_map()->get(fd);
    auto pos = adafs_fd->pos(); //retrieve the current offset
    if (adafs_fd->get_flag(OpenFile_flags::append))
        adafs_lseek(adafs_fd, 0, SEEK_END);
    auto ret = adafs_pwrite(adafs_fd, reinterpret_cast<const char*>(buf), count, pos);
    // Update offset in file descriptor in the file map
    if (ret > 0) {
        adafs_fd->pos(pos + count);
    }
    return ret;
}

ssize_t adafs_pwritev(int fd, const struct iovec *iov, int iovcnt, off_t offset) {
    CTX->log()->trace("{}() called with fd {}, op num {}, offset {}",
                      __func__, fd, iovcnt, offset);

    auto file = CTX->file_map()->get(fd);
    auto pos = offset; // keep truck of current position
    ssize_t written = 0;
    ssize_t ret;
    for (int i = 0; i < iovcnt; ++i) {
        auto count = (iov+i)->iov_len;
        if (count == 0) {
            continue;
        }
        auto buf = (iov+i)->iov_base;
        ret = adafs_pwrite(file, reinterpret_cast<char *>(buf), count, pos);
        if (ret == -1) {
            break;
        }
        written += ret;
        pos += ret;

        if (static_cast<size_t>(ret) < count) {
            break;
        }
    }

    if (written == 0) {
        return -1;
    }
    return written;
}

ssize_t adafs_writev(int fd, const struct iovec * iov, int iovcnt) {
    CTX->log()->trace("{}() called with fd {}, ops num {}",
            __func__, fd, iovcnt);
    auto adafs_fd = CTX->file_map()->get(fd);
    auto pos = adafs_fd->pos(); // retrieve the current offset
    auto ret = adafs_pwritev(fd, iov, iovcnt, pos);
    assert(ret != 0);
    if (ret < 0) {
        return -1;
    }
    adafs_fd->pos(pos + ret);
    return ret;
}

ssize_t adafs_pread(std::shared_ptr<OpenFile> file, char * buf, size_t count, off64_t offset) {
    if (file->type() != FileType::regular) {
        assert(file->type() == FileType::directory);
        CTX->log()->warn("{}() cannot read from directory", __func__);
        errno = EISDIR;
        return -1;
    }
    CTX->log()->trace("{}() count: {}, offset: {}", __func__, count, offset);
    // Zeroing buffer before read is only relevant for sparse files. Otherwise sparse regions contain invalid data.
#if defined(ZERO_BUFFER_BEFORE_READ)
    memset(buf, 0, sizeof(char)*count);
#endif
    auto ret = rpc_send::read(file->path(), buf, offset, count);
    if (ret < 0) {
        CTX->log()->warn("{}() rpc_send::read failed with ret {}", __func__, ret);
    }
    // XXX check that we don't try to read past end of the file
    return ret; // return read size or -1 as error
}

ssize_t adafs_read(int fd, void* buf, size_t count) {
    auto adafs_fd = CTX->file_map()->get(fd);
    auto pos = adafs_fd->pos(); //retrieve the current offset
    auto ret = adafs_pread(adafs_fd, reinterpret_cast<char*>(buf), count, pos);
    // Update offset in file descriptor in the file map
    if (ret > 0) {
        adafs_fd->pos(pos + ret);
    }
    return ret;
}

ssize_t adafs_pread_ws(int fd, void* buf, size_t count, off64_t offset) {
    auto adafs_fd = CTX->file_map()->get(fd);
    return adafs_pread(adafs_fd, reinterpret_cast<char*>(buf), count, offset);
}

int adafs_opendir(const std::string& path) {

    auto md = adafs_metadata(path);
    if (!md) {
        return -1;
    }
    if (!S_ISDIR(md->mode())) {
        CTX->log()->debug("{}() path is not a directory", __func__);
        errno = ENOTDIR;
        return -1;
    }

    auto open_dir = std::make_shared<OpenDir>(path);
    rpc_send::get_dirents(*open_dir);
    return CTX->file_map()->add(open_dir);
}

int adafs_rmdir(const std::string& path) {
    auto md = adafs_metadata(path);
    if (!md) {
        CTX->log()->debug("{}() path does not exists: '{}'", __func__, path);
        errno = ENOENT;
        return -1;
    }
    if (!S_ISDIR(md->mode())) {
        CTX->log()->debug("{}() path is not a directory", __func__);
        errno = ENOTDIR;
        return -1;
    }

    auto open_dir = std::make_shared<OpenDir>(path);
    rpc_send::get_dirents(*open_dir);
    if(open_dir->size() != 0){
        errno = ENOTEMPTY;
        return -1;
    }
    return rpc_send::rm_node(path, true, 0);
}


int getdents(unsigned int fd,
             struct linux_dirent *dirp,
             unsigned int count) {
    CTX->log()->trace("{}() called on fd: {}, count {}", __func__, fd, count);
    auto open_dir = CTX->file_map()->get_dir(fd);
    if(open_dir == nullptr){
        //Cast did not succeeded: open_file is a regular file
        errno = EBADF;
        return -1;
    }

    auto pos = open_dir->pos();
    if (pos >= open_dir->size()) {
        return 0;
    }

    unsigned int written = 0;
    struct linux_dirent * current_dirp = nullptr;
    while(pos < open_dir->size()) {
        DirEntry de = open_dir->getdent(pos);
        auto total_size = ALIGN(offsetof(struct linux_dirent, d_name) +
                de.name().size() + 3, sizeof(long));
        if (total_size > (count - written)) {
            //no enough space left on user buffer to insert next dirent
            break;
        }
        current_dirp = reinterpret_cast<struct linux_dirent *>(
                        reinterpret_cast<char*>(dirp) + written);
        current_dirp->d_ino = std::hash<std::string>()(
                open_dir->path() + "/" + de.name());

        current_dirp->d_reclen = total_size;

        *(reinterpret_cast<char*>(current_dirp) + total_size - 1) =
            ((de.type() == FileType::regular)? DT_REG : DT_DIR);

        CTX->log()->trace("{}() name {}: {}", __func__, pos, de.name());
        std::strcpy(&(current_dirp->d_name[0]), de.name().c_str());
        ++pos;
        current_dirp->d_off = pos;
        written += total_size;
    }

    if (written == 0) {
        errno = EINVAL;
        return -1;
    }
    open_dir->pos(pos);
    return written;
}


int getdents64(unsigned int fd,
             struct linux_dirent64 *dirp,
             unsigned int count) {
    CTX->log()->trace("{}() called on fd: {}, count {}", __func__, fd, count);
    auto open_dir = CTX->file_map()->get_dir(fd);
    if(open_dir == nullptr){
        //Cast did not succeeded: open_file is a regular file
        errno = EBADF;
        return -1;
    }

    auto pos = open_dir->pos();
    if (pos >= open_dir->size()) {
        return 0;
    }

    unsigned int written = 0;
    struct linux_dirent64 * current_dirp = nullptr;
    while(pos < open_dir->size()) {
        DirEntry de = open_dir->getdent(pos);
        auto total_size = ALIGN(offsetof(struct linux_dirent64, d_name) +
                de.name().size() + 3, sizeof(long));
        if (total_size > (count - written)) {
            //no enough space left on user buffer to insert next dirent
            break;
        }
        current_dirp = reinterpret_cast<struct linux_dirent64 *>(
                        reinterpret_cast<char*>(dirp) + written);
        current_dirp->d_ino = std::hash<std::string>()(
                open_dir->path() + "/" + de.name());

        current_dirp->d_reclen = total_size;
        current_dirp->d_type =  ((de.type() == FileType::regular)? DT_REG : DT_DIR);

        

        CTX->log()->trace("{}() name {}: {}", __func__, pos, de.name());
        std::strcpy(&(current_dirp->d_name[0]), de.name().c_str());
        ++pos;
        current_dirp->d_off = pos;
        written += total_size;
    }

    if (written == 0) {
        errno = EINVAL;
        return -1;
    }
    open_dir->pos(pos);
    return written;
}


#ifdef HAS_SYMLINKS

int adafs_mk_symlink(const std::string& path, const std::string& target_path) {
    init_ld_env_if_needed();
    /* The following check is not POSIX compliant.
     * In POSIX the target is not checked at all.
    *  Here if the target is a directory we raise a NOTSUP error.
    *  So that application know we don't support link to directory.
    */
    auto target_md = adafs_metadata(target_path, false);
    if (target_md != nullptr) {
        auto trg_mode = target_md->mode();
        if (!(S_ISREG(trg_mode) || S_ISLNK(trg_mode))) {
            assert(S_ISDIR(trg_mode));
            CTX->log()->debug("{}() target path is a directory. Not supported", __func__);
            errno = ENOTSUP;
            return -1;
        }
    }

    auto p_comp = dirname(path);
    auto md = adafs_metadata(p_comp, false);
    if (md == nullptr) {
        CTX->log()->debug("{}() parent component does not exist: '{}'", __func__, p_comp);
        errno = ENOENT;
        return -1;
    }
    if (!S_ISDIR(md->mode())) {
        CTX->log()->debug("{}() parent component is not a directory: '{}'", __func__, p_comp);
        errno = ENOTDIR;
        return -1;
    }

    auto link_md = adafs_metadata(path, false);
    if (link_md != nullptr) {
        CTX->log()->debug("{}() Link exists: '{}'", __func__, p_comp);
        errno = EEXIST;
        return -1;
    }

    return rpc_send::mk_symlink(path, target_path);
}

int adafs_readlink(const std::string& path, char *buf, int bufsize) {
    init_ld_env_if_needed();
    auto md = adafs_metadata(path, false);
    if (md == nullptr) {
        CTX->log()->debug("{}() named link doesn't exists", __func__);
        return -1;
    }
    if (!(md->is_link())) {
        CTX->log()->debug("{}() The named file is not a symbolic link", __func__);
        errno = EINVAL;
        return -1;
    }
    int path_size = md->target_path().size() + CTX->mountdir().size();
    if (path_size >= bufsize) {
        CTX->log()->warn("{}() destination buffer size is to short: {} < {}, {} ", __func__, bufsize, path_size, md->target_path());
        errno = ENAMETOOLONG;
        return -1;
    }

    CTX->mountdir().copy(buf, CTX->mountdir().size());
    std::strcpy(buf + CTX->mountdir().size(), md->target_path().c_str());
    return path_size;
}

#endif
