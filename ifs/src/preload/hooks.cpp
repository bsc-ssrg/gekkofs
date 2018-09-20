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

    if(cpath == nullptr || cpath[0] == '\0') {
        CTX->log()->error("{}() path is invalid", __func__);
        return -EINVAL;
    }

    CTX->log()->trace("{}() called with fd: {}, path: {}, flags: {}, mode: {}", __func__, dirfd, cpath, flags, mode);

    std::string resolved;

    if((cpath[0] == PSP) || (dirfd == AT_FDCWD)) {
        // cpath is absolute or relative to CWD
        if (CTX->relativize_path(cpath, resolved)) {
            int ret = adafs_open(resolved, mode, flags);
            if(ret < 0) {
                return -errno;
            }
        }
    } else {
        // cpath is relative
        if(!(CTX->file_map()->exist(dirfd))) {
            //TODO relative cpath could still lead to our FS
            return syscall_no_intercept(SYS_openat, dirfd, cpath, flags, mode);
        }

        auto dir = CTX->file_map()->get_dir(dirfd);
        if(dir == nullptr) {
            CTX->log()->error("{}() dirfd is not a directory ", __func__);
            errno = ENOTDIR;
            return -1;
        }

        std::string path = CTX->mountdir();
        path.append(dir->path());
        path.push_back(PSP);
        path.append(cpath);
        if(resolve_path(path, resolved)) {
            int ret = adafs_open(resolved, mode, flags);
            if(ret < 0) {
                return -errno;
            }
        }
    }
    return 0;
}
