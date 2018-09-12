#include "preload/hooks.hpp"
#include "preload/preload.hpp"
#include "preload/adafs_functions.hpp"
#include "preload/resolve.hpp"
#include "preload/open_dir.hpp"
#include "global/path_util.hpp"

#include <libsyscall_intercept_hook_point.h>
#include <sys/stat.h>
#include <fcntl.h>


static inline int with_errno(int ret) {
    return (ret < 0)? -errno : ret;
}


int hook_openat(int dirfd, const char *cpath, int flags, mode_t mode) {

    CTX->log()->trace("{}() called with fd: {}, path: {}, flags: {}, mode: {}",
                       __func__, dirfd, cpath, flags, mode);

    std::string resolved;
    auto rstatus = CTX->relativize_fd_path(dirfd, cpath, resolved);
    switch(rstatus) {
        case RelativizeStatus::fd_unknown:
            return syscall_no_intercept(SYS_openat, dirfd, cpath, flags, mode);

        case RelativizeStatus::external:
            return syscall_no_intercept(SYS_openat, dirfd, resolved.c_str(), flags, mode);

        case RelativizeStatus::fd_not_a_dir:
            return -ENOTDIR;

        case RelativizeStatus::internal:
            return with_errno(adafs_open(resolved, mode, flags));

        default:
            CTX->log()->error("{}() relativize status unknown: {}", __func__);
            return -EINVAL;
    }
}

int hook_close(int fd) {
    CTX->log()->trace("{}() called with fd {}", __func__, fd);
    if(CTX->file_map()->exist(fd)) {
        // No call to the daemon is required
        CTX->file_map()->remove(fd);
        return 0;
    }
    return syscall_no_intercept(SYS_close, fd);
}

int hook_stat(const char* path, struct stat* buf) {
    CTX->log()->trace("{}() called with path '{}'", __func__, path);
    std::string rel_path;
    if (CTX->relativize_path(path, rel_path, false)) {
            return with_errno(adafs_stat(rel_path, buf));
    }
    return syscall_no_intercept(SYS_stat, rel_path.c_str(), buf);
}

int hook_read(int fd, void* buf, size_t count) {
    CTX->log()->trace("{}() called with fd {}, count {}", __func__, fd, count);
    if (CTX->file_map()->exist(fd)) {
        auto ret = adafs_read(fd, buf, count);
        if(ret < 0) {
            return -errno;
        }
        return ret;
    }
    return syscall_no_intercept(SYS_read, fd, buf, count);
}

int hook_write(int fd, void* buf, size_t count) {
    CTX->log()->trace("{}() called with fd {}, count {}", __func__, fd, count);
    if (CTX->file_map()->exist(fd)) {
        auto ret = adafs_write(fd, buf, count);
        if(ret < 0) {
            return -errno;
        }
        return ret;
    }
    return syscall_no_intercept(SYS_write, fd, buf, count);
}

int hook_unlink(const char* path) {
    CTX->log()->trace("{}() called with path '{}'", __func__, path);
    std::string rel_path;
    if (CTX->relativize_path(path, rel_path)) {
        auto ret = adafs_rm_node(rel_path);
        if(ret < 0) {
            return -errno;
        }
        return ret;
    }
    return syscall_no_intercept(SYS_unlink, rel_path.c_str());
}
