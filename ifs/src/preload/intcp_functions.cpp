/**
 * All intercepted functions are defined here
 */
#include <sys/statfs.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/unistd.h>

#include <preload/preload.hpp>
#include <preload/resolve.hpp>
#include <preload/passthrough.hpp>
#include <preload/adafs_functions.hpp>
#include <preload/intcp_functions.hpp>
#include <preload/open_dir.hpp>
#include <global/path_util.hpp>


using namespace std;

void inline notsup_error_32_bit_func(const char* func = __builtin_FUNCTION()) {
    CTX->log()->error("{}() is NOT SUPPORTED. According to glibc, this function should be called only on 32-bit machine", func);
}

int intcp_open(const char* path, int flags, ...) {
    init_passthrough_if_needed();

    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list vl;
        va_start(vl, flags);
        mode = static_cast<mode_t>(va_arg(vl, int));
        va_end(vl);
    }
    if (!CTX->initialized()) {
        return LIBC_FUNC(open, path, flags, mode);
    }

    CTX->log()->trace("{}() called with path '{}'", __func__, path);
    std::string rel_path;
    if (!CTX->relativize_path(path, rel_path)) {
        return LIBC_FUNC(open, rel_path.c_str(), flags, mode);
    }
    return adafs_open(rel_path, mode, flags);
}

#undef open64

int intcp_open64(const char* path, int flags, ...) {
    init_passthrough_if_needed();
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = va_arg(ap, mode_t);
        va_end(ap);
    }
    return intcp_open(path, flags | O_LARGEFILE, mode);
}

int intcp_openat(int dirfd, const char *cpath, int flags, ...) {
    init_passthrough_if_needed();

    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list vl;
        va_start(vl, flags);
        mode = static_cast<mode_t>(va_arg(vl, int));
        va_end(vl);
    }

    if(!CTX->initialized()) {
        return LIBC_FUNC(openat, dirfd, cpath, flags, mode);
    }

    if(cpath == nullptr || cpath[0] == '\0') {
        CTX->log()->error("{}() path is invalid", __func__);
        errno = EINVAL;
        return -1;
    }

    CTX->log()->trace("{}() called with fd: {}, path: {}, flags: {}, mode: {}", __func__, dirfd, cpath, flags, mode);

    std::string resolved;

    if((cpath[0] == PSP) || (dirfd == AT_FDCWD)) {
        // cpath is absolute or relative to CWD
        if (CTX->relativize_path(cpath, resolved)) {
            return adafs_open(resolved, mode, flags);
        }
    } else {
        // cpath is relative
        if(!(CTX->file_map()->exist(dirfd))) {
            //TODO relative cpath could still lead to our FS
            return LIBC_FUNC(openat, dirfd, cpath, flags, mode);
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
            return adafs_open(resolved, mode, flags);
        }
    }
    return LIBC_FUNC(openat, dirfd, resolved.c_str(), flags, mode);
}

int intcp_openat64(int dirfd, const char *path, int flags, ...) {
    init_passthrough_if_needed();

    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list vl;
        va_start(vl, flags);
        mode = static_cast<mode_t>(va_arg(vl, int));
        va_end(vl);
    }

    return intcp_openat(dirfd, path, flags | O_LARGEFILE, mode);
}

/******  FILE OPS  ******/

inline int file_to_fd(const FILE* f){
    assert(f != nullptr);
    return *(reinterpret_cast<int*>(&f));
}

inline FILE* fd_to_file(const int fd){
    assert(fd >= 0);
    return reinterpret_cast<FILE*>(fd);
}

FILE* fopen(const char* path, const char* fmode) {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(fopen, path, fmode);
    }
    CTX->log()->trace("{}() called with path '{}' with mode '{}'", __func__, path, fmode);
    std::string rel_path;
    if (!CTX->relativize_path(path, rel_path)) {
        return LIBC_FUNC(fopen, rel_path.c_str(), fmode);
    }
    int flags = 0;
    std::string str_mode(fmode);
    str_mode.erase(std::remove(str_mode.begin(), str_mode.end(), 'b'), str_mode.end());

    if(str_mode == "r") {
        flags = O_RDONLY;
    } else if(str_mode == "r+") {
        flags = O_RDWR;
    } else if(str_mode == "w") {
        flags = (O_WRONLY | O_CREAT | O_TRUNC);
    } else if(str_mode == "w+") {
        flags = (O_RDWR | O_CREAT | O_TRUNC);
    } else if(str_mode == "a") {
        flags = (O_WRONLY | O_CREAT | O_APPEND);
    } else if(str_mode == "a+") {
        flags = (O_RDWR | O_CREAT | O_APPEND);
    } else {
        CTX->log()->error("{}() stream open flags NOT SUPPORTED: '{}'", __func__, str_mode);
        errno = ENOTSUP;
        return nullptr;
    }
    mode_t mode = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    auto fd = adafs_open(rel_path, mode, flags);
    if(fd == -1){
        return nullptr;
    } else {
        return fd_to_file(fd);
    }
}

FILE* fopen64(const char* path, const char* fmode) {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(fopen64, path, fmode);
    }
    CTX->log()->trace("{}() called with path '{}' with mode '{}'", __func__, path, fmode);
    std::string rel_path;
    if (!CTX->relativize_path(path, rel_path)) {
        return LIBC_FUNC(fopen64, rel_path.c_str(), fmode);
    }
    return fopen(path, fmode);
}

size_t intcp_fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    init_passthrough_if_needed();
    if(CTX->initialized() && (stream != nullptr)) {
        auto fd = file_to_fd(stream);
        if (CTX->file_map()->exist(fd)) {
            CTX->log()->trace("{}() called with fd {}", __func__, fd);
            auto ret = adafs_read(fd, ptr, size*nmemb);
            if (ret > 0) {
                return ret / size;
            }
            return static_cast<size_t>(ret);
        }
    }
    return (reinterpret_cast<decltype(&fread)>(libc_fread))(ptr, size, nmemb, stream);
}

size_t intcp_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    init_passthrough_if_needed();
    if(CTX->initialized() && (stream != nullptr)) {
        auto fd = file_to_fd(stream);
        if (CTX->file_map()->exist(fd)) {
            CTX->log()->trace("{}() called with fd {}", __func__, fd);
            auto ret = adafs_write(fd, ptr, size*nmemb);
            if (ret > 0) {
                // Update offset in file descriptor in the file map
                return ret / size;
            }
            return static_cast<size_t>(ret);
        }
    }
    return (reinterpret_cast<decltype(&fwrite)>(libc_fwrite))(ptr, size, nmemb, stream);
}

int fclose(FILE *stream) {
    init_passthrough_if_needed();
    if(CTX->initialized() && (stream != nullptr)) {
        auto fd = file_to_fd(stream);
        if(CTX->file_map()->exist(fd)) {
            // No call to the daemon is required
            CTX->file_map()->remove(fd);
            return 0;
        }
    }
    return (reinterpret_cast<decltype(&fclose)>(libc_fclose))(stream);
}

int fileno(FILE *stream) {
    init_passthrough_if_needed();
    if(CTX->initialized() && (stream != nullptr)) {
        auto fd = file_to_fd(stream);
        if(CTX->file_map()->exist(fd)) {
            CTX->log()->trace("{}() called with fd {}", __func__, fd);
            return fd;
        }
    }
    return (reinterpret_cast<decltype(&fileno)>(libc_fileno))(stream);
}

void clearerr(FILE *stream) {
    init_passthrough_if_needed();
    if(CTX->initialized() && (stream != nullptr)) {
        auto fd = file_to_fd(stream);
        if(CTX->file_map()->exist(fd)) {
            CTX->log()->error("{}() NOT SUPPORTED", __func__);
            errno = ENOTSUP;
            return;
        }
    }
    return (reinterpret_cast<decltype(&clearerr)>(libc_clearerr))(stream);
}

int feof(FILE *stream) {
    init_passthrough_if_needed();
    if(CTX->initialized() && (stream != nullptr)) {
        auto fd = file_to_fd(stream);
        if(CTX->file_map()->exist(fd)) {
            CTX->log()->error("{}() NOT SUPPORTED", __func__);
            errno = ENOTSUP;
            return -1;
        }
    }
    return (reinterpret_cast<decltype(&feof)>(libc_feof))(stream);
}

int ferror(FILE *stream) {
    init_passthrough_if_needed();
    if(CTX->initialized() && (stream != nullptr)) {
        auto fd = file_to_fd(stream);
        if(CTX->file_map()->exist(fd)) {
            CTX->log()->error("{}() NOT SUPPORTED", __func__);
            errno = ENOTSUP;
            return -1;
        }
    }
    return (reinterpret_cast<decltype(&ferror)>(libc_ferror))(stream);
}

int fflush(FILE *stream) {
    init_passthrough_if_needed();
    if(CTX->initialized() && (stream != nullptr)) {
        auto fd = file_to_fd(stream);
        if(CTX->file_map()->exist(fd)) {
            CTX->log()->trace("{}() called with fd {}", __func__, fd);
            return 0;
        }
    }
    return (reinterpret_cast<decltype(&fflush)>(libc_fflush))(stream);
}

int fpurge(FILE *stream) {
    init_passthrough_if_needed();
    if(CTX->initialized() && (stream != nullptr)) {
        auto fd = file_to_fd(stream);
        if(CTX->file_map()->exist(fd)) {
            CTX->log()->trace("{}() called on fd {}", __func__, fd);
            return 0;
        }
    }
    return (reinterpret_cast<decltype(&fpurge)>(libc_fpurge))(stream);
}

void __fpurge(FILE *stream) {
    init_passthrough_if_needed();
    if(CTX->initialized() && (stream != nullptr)) {
        auto fd = file_to_fd(stream);
        if(CTX->file_map()->exist(fd)) {
            CTX->log()->trace("{}() called on fd {}", __func__, fd);
            return;
        }
    }
    return (reinterpret_cast<decltype(&__fpurge)>(libc___fpurge))(stream);
}

void setbuf(FILE *stream, char *buf) {
    init_passthrough_if_needed();
    if(CTX->initialized() && (stream != nullptr)) {
        auto fd = file_to_fd(stream);
        if(CTX->file_map()->exist(fd)) {
            CTX->log()->trace("{}() called on fd {}", __func__, fd);
            return;
        }
    }
    return (reinterpret_cast<decltype(&setbuf)>(libc_setbuf))(stream, buf);
}

void setbuffer(FILE *stream, char *buf, size_t size) {
    init_passthrough_if_needed();
    if(CTX->initialized() && (stream != nullptr)) {
        auto fd = file_to_fd(stream);
        if(CTX->file_map()->exist(fd)) {
            CTX->log()->trace("{}() called on fd {}", __func__, fd);
            return;
        }
    }
    return (reinterpret_cast<decltype(&setbuffer)>(libc_setbuffer))(stream, buf, size);
}

void setlinebuf(FILE *stream) {
    init_passthrough_if_needed();
    if(CTX->initialized() && (stream != nullptr)) {
        auto fd = file_to_fd(stream);
        if(CTX->file_map()->exist(fd)) {
            CTX->log()->trace("{}() called on fd {}", __func__, fd);
            return;
        }
    }
    return (reinterpret_cast<decltype(&setlinebuf)>(libc_setlinebuf))(stream);
}

int setvbuf(FILE *stream, char *buf, int mode, size_t size) {
    init_passthrough_if_needed();
    if(CTX->initialized() && (stream != nullptr)) {
        auto fd = file_to_fd(stream);
        if(CTX->file_map()->exist(fd)) {
            CTX->log()->trace("{}() called on fd {}", __func__, fd);
            return 0;
        }
    }
    return (reinterpret_cast<decltype(&setvbuf)>(libc_setvbuf))(stream, buf, mode, size);
}

int putc(int c, FILE *stream) {
    init_passthrough_if_needed();
    if(CTX->initialized() && (stream != nullptr)) {
        auto fd = file_to_fd(stream);
        if(CTX->file_map()->exist(fd)) {
            CTX->log()->error("{}() NOT SUPPORTED", __func__);
            errno = ENOTSUP;
            return EOF;
        }
    }
    return LIBC_FUNC(putc, c, stream);
}

int fputc(int c, FILE *stream) {
    init_passthrough_if_needed();
    if(CTX->initialized() && (stream != nullptr)) {
        auto fd = file_to_fd(stream);
        if(CTX->file_map()->exist(fd)) {
            CTX->log()->error("{}() NOT SUPPORTED", __func__);
            errno = ENOTSUP;
            return EOF;
        }
    }
    return LIBC_FUNC(fputc, c, stream);
}

int fputs(const char *s, FILE *stream) {
    init_passthrough_if_needed();
    if(CTX->initialized() && (stream != nullptr)) {
        auto fd = file_to_fd(stream);
        if(CTX->file_map()->exist(fd)) {
            CTX->log()->error("{}() NOT SUPPORTED", __func__);
            errno = ENOTSUP;
            return EOF;
        }
    }
    return LIBC_FUNC(fputs, s, stream);
}

int getc(FILE *stream) {
    init_passthrough_if_needed();
    if(CTX->initialized() && (stream != nullptr)) {
        auto fd = file_to_fd(stream);
        if(CTX->file_map()->exist(fd)) {
            CTX->log()->error("{}() NOT SUPPORTED", __func__);
            errno = ENOTSUP;
            return EOF;
        }
    }
    return LIBC_FUNC(getc, stream);
}

int fgetc(FILE *stream) {
    init_passthrough_if_needed();
    if(CTX->initialized() && (stream != nullptr)) {
        auto fd = file_to_fd(stream);
        if(CTX->file_map()->exist(fd)) {
            CTX->log()->error("{}() NOT SUPPORTED", __func__);
            errno = ENOTSUP;
            return EOF;
        }
    }
    return LIBC_FUNC(fgetc, stream);
}

char* fgets(char* s, int size, FILE* stream) {
    init_passthrough_if_needed();
    if(CTX->initialized() && (stream != nullptr)) {
        auto fd = file_to_fd(stream);
        if(CTX->file_map()->exist(fd)) {
            CTX->log()->trace("{}() [fd: {}, size: {}]", __func__, fd, size);
            auto ret = adafs_read(fd, s, static_cast<size_t>(size - 1));
            CTX->log()->debug("{}() read {} bytes", __func__, ret);
            if(ret > 0) {
                char* nl_ptr = static_cast<char*>(memchr(s, '\n', static_cast<size_t>(size - 1)));
                assert((nl_ptr - s) < size);
                if(nl_ptr != nullptr) {
                    CTX->log()->debug("{}() found new line char at {}", __func__, (nl_ptr - s));
                    nl_ptr[1] = '\0';
                } else {
                    s[size - 1] = '\0';
                }
                return s;
            } else {
                return nullptr;
            }
        }
    }
    return LIBC_FUNC(fgets, s, size, stream);
}

int ungetc(int c, FILE *stream) {
    init_passthrough_if_needed();
    if(CTX->initialized() && (stream != nullptr)) {
        auto fd = file_to_fd(stream);
        if(CTX->file_map()->exist(fd)) {
            CTX->log()->error("{}() NOT SUPPORTED", __func__);
            errno = ENOTSUP;
            return EOF;
        }
    }
    return LIBC_FUNC(ungetc, c, stream);
}

int fseek(FILE *stream, long offset, int whence) {
    init_passthrough_if_needed();
    if(CTX->initialized() && (stream != nullptr)) {
        auto fd = file_to_fd(stream);
        if(CTX->file_map()->exist(fd)) {
            CTX->log()->trace("{}() called [fd: {}, offset: {}, whence: {}");
            if(adafs_lseek(fd, offset, whence) == -1) {
                return -1;
            } else {
                return 0;
            }
        }
    }
    return LIBC_FUNC(fseek, stream, offset, whence);
}

/******  FILE OPS  ******/

#undef creat

int creat(const char* path, mode_t mode) {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        CTX->log()->trace("{}() called with path '{}', mode '{}'", __func__, path, mode);
    }
    return open(path, O_CREAT | O_WRONLY | O_TRUNC, mode);
}

#undef creat64

int creat64(const char* path, mode_t mode) {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        CTX->log()->trace("{}() called with path '{}', mode '{}'", __func__, path, mode);
    }
    return open(path, O_CREAT | O_WRONLY | O_TRUNC | O_LARGEFILE, mode);
}

int mkdir(const char* path, mode_t mode) noexcept {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(mkdir, path, mode);
    }
    CTX->log()->trace("{}() called with path '{}', mode '{}'", __func__, path, mode);
    std::string rel_path;
    if(!CTX->relativize_path(path, rel_path)) {
        return LIBC_FUNC(mkdir, rel_path.c_str(), mode);
    }
    auto ret = adafs_mk_node(rel_path, mode | S_IFDIR);
    CTX->log()->trace("{}() ret {}", __func__, ret);
    return ret;
}

int mkdirat(int dirfd, const char* path, mode_t mode) noexcept {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        CTX->log()->trace("{}() called with path '{}', mode {}, dirfd {}", __func__, path, mode, dirfd);
        std::string rel_path;
        if (CTX->relativize_path(path, rel_path)) {
            // not implemented
            CTX->log()->error("{}() not implemented.", __func__);
            errno = ENOTSUP;
            return -1;
        }
    }
    return LIBC_FUNC(mkdirat, dirfd, path, mode);
}


int unlink(const char* path) noexcept {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(unlink, path);
    }
    CTX->log()->trace("{}() called with path '{}'", __func__, path);
    std::string rel_path;
    if (!CTX->relativize_path(path, rel_path, false)) {
        return LIBC_FUNC(unlink, rel_path.c_str());
    }
    return adafs_rm_node(rel_path);
}

int unlinkat(int dirfd, const char* cpath, int flags) noexcept {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(unlinkat, dirfd, cpath, flags);
    }

    if (cpath[0] == '\0') {
        CTX->log()->error("{}() path is invalid", __func__);
        errno = EINVAL;
        return -1;
    }

    CTX->log()->trace("{}() called with path '{}' dirfd {}, flags {}", __func__, cpath, dirfd, flags);

    std::string resolved;

    if(cpath[0] != PSP) {
        if(!(CTX->file_map()->exist(dirfd))) {
            //TODO relative cpath could still lead to our FS
            return LIBC_FUNC(unlinkat, dirfd, cpath, flags);
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
        if (!resolve_path(path, resolved)) {
            return LIBC_FUNC(unlinkat, dirfd, resolved.c_str(), flags);
        }
    } else {
        if (!CTX->relativize_path(cpath, resolved)) {
            return LIBC_FUNC(unlinkat, dirfd, resolved.c_str(), flags);
        }
    }

    if(flags & AT_REMOVEDIR) {
        return adafs_rmdir(resolved);
    } else {
        return adafs_rm_node(resolved);
    }
}

int rmdir(const char* path) noexcept {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(rmdir, path);
    }
    CTX->log()->trace("{}() called with path '{}'", __func__, path);
    std::string rel_path;
    if (!CTX->relativize_path(path, rel_path)) {
        return LIBC_FUNC(rmdir, rel_path.c_str());
    }
    return adafs_rmdir(rel_path);
}

int close(int fd) {
    init_passthrough_if_needed();
    if(CTX->initialized() && CTX->file_map()->exist(fd)) {
        // No call to the daemon is required
        CTX->file_map()->remove(fd);
        return 0;
    }
    return LIBC_FUNC(close, fd);
}

int __close(int fd) {
    return close(fd);
}

int remove(const char* path) {
   return unlink(path);
}

int access(const char* path, int mask) noexcept {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(access, path, mask);
    }
    CTX->log()->trace("{}() called path '{}', mask {}", __func__, path, mask);
    std::string rel_path;
    if (!CTX->relativize_path(path, rel_path)) {
        return LIBC_FUNC(access, rel_path.c_str(), mask);
    }
    return adafs_access(rel_path, mask);
}

int faccessat(int dirfd, const char* cpath, int mode, int flags) noexcept {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(faccessat, dirfd, cpath, mode, flags);
    }

    if (cpath[0] == '\0') {
        CTX->log()->error("{}() path is invalid", __func__);
        errno = EINVAL;
        return -1;
    }

    CTX->log()->trace("{}() called path '{}', mode {}, dirfd {}, flags {}", __func__, cpath, mode, dirfd, flags);

    std::string resolved;

    if((cpath[0] == PSP) || (dirfd == AT_FDCWD)) {
        // cpath is absolute or relative to CWD
        if (CTX->relativize_path(cpath, resolved)) {
            return adafs_access(resolved, mode);
        }
    } else {
        // cpath is relative
        if(!(CTX->file_map()->exist(dirfd))) {
            //TODO relative cpath could still lead to our FS
            return LIBC_FUNC(faccessat, dirfd, cpath, mode, flags);
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
            return adafs_access(resolved, mode);
        }
    }
    return LIBC_FUNC(faccessat, dirfd, resolved.c_str(), mode, flags);
}


int stat(const char* path, struct stat* buf) noexcept {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(stat, path, buf);
    }
    CTX->log()->trace("{}() called with path '{}'", __func__, path);
    std::string rel_path;
    if (!CTX->relativize_path(path, rel_path)) {
        return LIBC_FUNC(stat, rel_path.c_str(), buf);
    }
    return adafs_stat(rel_path, buf);
}

int fstat(int fd, struct stat* buf) noexcept {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        CTX->log()->trace("{}() called with fd {}", __func__, fd);
        if (CTX->file_map()->exist(fd)) {
            auto path = CTX->file_map()->get(fd)->path();
            return adafs_stat(path, buf);
        }
    }
    return LIBC_FUNC(fstat, fd, buf);
}

int lstat(const char* path, struct stat* buf) noexcept {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(lstat, path, buf);
    }
    CTX->log()->trace("{}() called with path '{}'", __func__, path);
    std::string rel_path;
    if (!CTX->relativize_path(path, rel_path, false)) {
        return LIBC_FUNC(lstat, rel_path.c_str(), buf);
    }
    CTX->log()->warn("{}() No symlinks are supported. Stats will always target the given path", __func__);
    return adafs_stat(rel_path, buf);
}

int __xstat(int ver, const char* path, struct stat* buf) noexcept {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(__xstat, ver, path, buf);
    }
    CTX->log()->trace("{}() called with path '{}'", __func__, path);
    std::string rel_path;
    if (!CTX->relativize_path(path, rel_path)) {
        return LIBC_FUNC(__xstat, ver, rel_path.c_str(), buf);
    }
    return adafs_stat(rel_path, buf);
}

int __xstat64(int ver, const char* path, struct stat64* buf) noexcept {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(__xstat64, ver, path, buf);
    }
    CTX->log()->trace("{}() called with path '{}'", __func__, path);
    std::string rel_path;
    if (!CTX->relativize_path(path, rel_path)) {
        return LIBC_FUNC(__xstat64, ver, rel_path.c_str(), buf);
    }
    notsup_error_32_bit_func();
    errno = ENOTSUP;
    return -1;
}

int __fxstat(int ver, int fd, struct stat* buf) noexcept {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        CTX->log()->trace("{}() called with fd {}", __func__, fd);
        if (CTX->file_map()->exist(fd)) {
            auto path = CTX->file_map()->get(fd)->path();
            return adafs_stat(path, buf);
        }
    }
    return LIBC_FUNC(__fxstat, ver, fd, buf);
}

int __fxstatat(int ver, int dirfd, const char* cpath, struct stat* buf, int flags) noexcept {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(__fxstatat, ver, dirfd, cpath, buf, flags);
    }

    if (cpath[0] == '\0') {
        CTX->log()->error("{}() path is invalid", __func__);
        errno = EINVAL;
        return -1;
    }

    CTX->log()->trace("{}() called with path '{}' and fd {}", __func__, cpath, dirfd);

    if(flags & AT_EMPTY_PATH) {
        CTX->log()->error("{}() AT_EMPTY_PATH flag not supported", __func__);
        errno = ENOTSUP;
        return -1;
    }

    std::string resolved;

    if(cpath[0] != PSP) {
        // cpath is relative
        //TODO handle the case in which dirfd is AT_FDCWD
        if(!(CTX->file_map()->exist(dirfd))) {
            //TODO relative cpath could still lead to our FS
            return LIBC_FUNC(__fxstatat, ver, dirfd, cpath, buf, flags);
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
            return adafs_stat(resolved, buf);
        }
    } else {
        // Path is absolute

        if (CTX->relativize_path(cpath, resolved)) {
            return adafs_stat(resolved, buf);
        }
    }
    return LIBC_FUNC(__fxstatat, ver, dirfd, cpath, buf, flags);
}

int __fxstatat64(int ver, int dirfd, const char* path, struct stat64* buf, int flags) noexcept {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(__fxstatat64, ver, dirfd, path, buf, flags);
    }

    notsup_error_32_bit_func();
    errno = ENOTSUP;
    return -1;
}

int __fxstat64(int ver, int fd, struct stat64* buf) noexcept {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(__fxstat64, ver, fd, buf);
    }

    notsup_error_32_bit_func();
    errno = ENOTSUP;
    return -1;
}

int __lxstat(int ver, const char* path, struct stat* buf) noexcept {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(__lxstat, ver, path, buf);
    }
    CTX->log()->trace("{}() called with path '{}'", __func__, path);
    std::string rel_path;
    if (!CTX->relativize_path(path, rel_path, false)) {
        return LIBC_FUNC(__lxstat, ver, path, buf);
    }
    return adafs_stat(rel_path, buf);
}

int __lxstat64(int ver, const char* path, struct stat64* buf) noexcept {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(__lxstat64, ver, path, buf);
    }

    notsup_error_32_bit_func();
    errno = ENOTSUP;
    return -1;
}

int statfs(const char* path, struct statfs* buf) noexcept {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(statfs, path, buf);
    }
    CTX->log()->trace("{}() called with path '{}'", __func__, path);
    std::string rel_path;
    if (!CTX->relativize_path(path, rel_path)) {
        return LIBC_FUNC(statfs, path, buf);
    }

    // get information of the underlying fs.
    // Note, we explicitely call the real glibc statfs function to not intercept it again on the mountdir path
    struct statfs realfs{};
    auto ret = LIBC_FUNC(statfs, CTX->mountdir().c_str(), &realfs);
    if (ret != 0) {
        return ret;
    }
    return adafs_statfs(rel_path, buf, realfs);
}

int fstatfs(int fd, struct statfs* buf) noexcept {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        CTX->log()->trace("{}() called with fd {}", __func__, fd);
        if (CTX->file_map()->exist(fd)) {
            auto adafs_fd = CTX->file_map()->get(fd);
            // get information of the underlying fs
            // Note, we explicitely call the real glibc statfs function to not intercept it again on the mountdir path
            struct statfs realfs{};
            auto ret = (reinterpret_cast<decltype(&statfs)>(libc_statfs))(CTX->mountdir().c_str(), &realfs);
            if (ret != 0)
                return ret;
            return adafs_statfs(adafs_fd->path(), buf, realfs);
        }
    }
    return (reinterpret_cast<decltype(&fstatfs)>(libc_fstatfs))(fd, buf);
}

ssize_t write(int fd, const void* buf, size_t count) {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        CTX->log()->trace("{}() called with fd {}", __func__, fd);
        if (CTX->file_map()->exist(fd)) {
            return adafs_write(fd, buf, count);
        }
    }
    return (reinterpret_cast<decltype(&write)>(libc_write))(fd, buf, count);
}

ssize_t pwrite(int fd, const void* buf, size_t count, off_t offset) {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        CTX->log()->trace("{}() called with fd {}", __func__, fd);
        if (CTX->file_map()->exist(fd)) {
            return adafs_pwrite_ws(fd, buf, count, offset);
        }
    }
    return (reinterpret_cast<decltype(&pwrite)>(libc_pwrite))(fd, buf, count, offset);
}

ssize_t pwrite64(int fd, const void* buf, size_t count, __off64_t offset) {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        CTX->log()->trace("{}() called with fd {}", __func__, fd);
        if (CTX->file_map()->exist(fd)) {
            return adafs_pwrite_ws(fd, buf, count, offset);
        }
    }
    return (reinterpret_cast<decltype(&pwrite64)>(libc_pwrite64))(fd, buf, count, offset);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        CTX->log()->trace("{}() called with fd {}", __func__, fd);
        if (CTX->file_map()->exist(fd)) {
            return adafs_writev(fd, iov, iovcnt);
        }
    }
    return (reinterpret_cast<decltype(&writev)>(libc_writev))(fd, iov, iovcnt);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        CTX->log()->trace("{}() called with fd {}", __func__, fd);
        if (CTX->file_map()->exist(fd)) {
            CTX->log()->error("{}() NOT SUPPORTED", __func__);
            errno = ENOTSUP;
            return -1;
        }
    }
    return (reinterpret_cast<decltype(&readv)>(libc_readv))(fd, iov, iovcnt);
}

ssize_t read(int fd, void* buf, size_t count) {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        CTX->log()->trace("{}() called with fd {}, count {}", __func__, fd, count);
        if (CTX->file_map()->exist(fd)) {
            auto ret = adafs_read(fd, buf, count);
            CTX->log()->trace("{}() returning {}", __func__, ret);
            return ret;
        }
    }
    return (reinterpret_cast<decltype(&read)>(libc_read))(fd, buf, count);
}

ssize_t pread(int fd, void* buf, size_t count, off_t offset) {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        CTX->log()->trace("{}() called with fd {}", __func__, fd);
        if (CTX->file_map()->exist(fd)) {
            return adafs_pread_ws(fd, buf, count, offset);
        }
    }
    return (reinterpret_cast<decltype(&pread)>(libc_pread))(fd, buf, count, offset);
}

ssize_t pread64(int fd, void* buf, size_t count, __off64_t offset) {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        CTX->log()->trace("{}() called with fd {}", __func__, fd);
        if (CTX->file_map()->exist(fd)) {
            return adafs_pread_ws(fd, buf, count, offset);
        }
    }
    return (reinterpret_cast<decltype(&pread64)>(libc_pread64))(fd, buf, count, offset);
}

off_t lseek(int fd, off_t offset, int whence) noexcept {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        CTX->log()->trace("{}() called with path '{}', mode {}", __func__, fd, offset, whence);
        if (CTX->file_map()->exist(fd)) {
            auto off_ret = adafs_lseek(fd, static_cast<off64_t>(offset), whence);
            if (off_ret > std::numeric_limits<off_t>::max()) {
                errno = EOVERFLOW;
                return -1;
            } else {
                return off_ret;
            }
        }
    }
   return (reinterpret_cast<decltype(&lseek)>(libc_lseek))(fd, offset, whence);
}

#undef lseek64

off64_t lseek64(int fd, off64_t offset, int whence) noexcept {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        CTX->log()->trace("{}() called with path '{}', mode {}", __func__, fd, offset, whence);
        if (CTX->file_map()->exist(fd)) {
            return adafs_lseek(fd, offset, whence);
        }
    }
    return (reinterpret_cast<decltype(&lseek64)>(libc_lseek64))(fd, offset, whence);
}

int fsync(int fd) {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        CTX->log()->trace("{}() called with fd '{}'", __func__, fd);
        if (CTX->file_map()->exist(fd)) {
            return 0; // This is a noop for us atm. fsync is called implicitly because each chunk is closed after access
        }
    }
    return (reinterpret_cast<decltype(&fsync)>(libc_fsync))(fd);
}

int fdatasync(int fd) {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        CTX->log()->trace("{}() called with fd '{}'", __func__, fd);
        if (CTX->file_map()->exist(fd)) {
            return 0; // This is a noop for us atm. fsync is called implicitly because each chunk is closed after access
        }
    }
    return (reinterpret_cast<decltype(&fdatasync)>(libc_fdatasync))(fd);
}

int truncate(const char* path, off_t length) noexcept {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(truncate, path, length);
    }
    CTX->log()->trace("{}() called with path: {}, offset: {}", __func__,
            path, length);
    std::string rel_path;
    if (!CTX->relativize_path(path, rel_path)) {
        return LIBC_FUNC(truncate, rel_path.c_str(), length);
    }
    return adafs_truncate(rel_path, length);
}

int ftruncate(int fd, off_t length) noexcept {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        CTX->log()->trace("{}() called  [fd: {}, offset: {}]", __func__, fd, length);
        if (CTX->file_map()->exist(fd)) {
            auto path = CTX->file_map()->get(fd)->path();
            return adafs_truncate(path, length);
        }
    }
    return (reinterpret_cast<decltype(&ftruncate)>(libc_ftruncate))(fd, length);
}

int fcntl(int fd, int cmd, ...) {
    init_passthrough_if_needed();
    va_list ap;
    void *arg;

    va_start (ap, cmd);
    arg = va_arg (ap, void *);
    va_end (ap);

    if(CTX->initialized()) {
        CTX->log()->trace("{}() called with fd {}", __func__, fd);
        if (CTX->file_map()->exist(fd)) {
            switch(cmd) {
                case F_DUPFD:
                    CTX->log()->trace("{}() F_DUPFD on fd {}", __func__, fd);
                    return adafs_dup(fd);
                case F_DUPFD_CLOEXEC: {
                    CTX->log()->trace("{}() F_DUPFD_CLOEXEC on fd {}", __func__, fd);
                    auto ret = adafs_dup(fd);
                    if(ret == -1) {
                        return ret;
                    }
                    CTX->file_map()->get(fd)
                        ->set_flag(OpenFile_flags::cloexec, true);
                    return ret;
                }
                case F_GETFD:
                    CTX->log()->trace("{}() F_GETFD on fd {}", __func__, fd);
                    if(CTX->file_map()->get(fd)
                            ->get_flag(OpenFile_flags::cloexec)) {
                        return FD_CLOEXEC;
                    } else {
                        return 0;
                    }
                case F_SETFD: {
                    va_start (ap, cmd);
                    int flags = va_arg (ap, int);
                    va_end (ap);
                    CTX->log()->trace("{}() [fd: {}, cmd: F_SETFD, FD_CLOEXEC: {}]", __func__, fd, (flags & FD_CLOEXEC));
                    CTX->file_map()->get(fd)
                            ->set_flag(OpenFile_flags::cloexec, static_cast<bool>(flags & FD_CLOEXEC));
                    return 0;
                }
                default:
                    CTX->log()->error("{}() unrecognized command {} on fd {}", __func__, cmd, fd);
                    errno = ENOTSUP;
                    return -1;
            }
        }
    }
    return (reinterpret_cast<decltype(&fcntl)>(libc_fcntl))(fd, cmd, arg);
}

int dup(int oldfd) noexcept {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        CTX->log()->trace("{}() called with oldfd {}", __func__, oldfd);
        if (CTX->file_map()->exist(oldfd)) {
            return adafs_dup(oldfd);
        }
    }
    return (reinterpret_cast<decltype(&dup)>(libc_dup))(oldfd);
}

int dup2(int oldfd, int newfd) noexcept {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        CTX->log()->trace("{}() called with oldfd {} newfd {}", __func__, oldfd, newfd);
        if (CTX->file_map()->exist(oldfd)) {
            return adafs_dup2(oldfd, newfd);
        }
    }
    return (reinterpret_cast<decltype(&dup2)>(libc_dup2))(oldfd, newfd);
}

int dup3(int oldfd, int newfd, int flags) noexcept {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        if (CTX->file_map()->exist(oldfd)) {
            // TODO implement O_CLOEXEC flag first which is used with fcntl(2)
            // It is in glibc since kernel 2.9. So maybe not that important :)
            CTX->log()->error("{}() Not implemented.", __func__);
            errno = EBUSY;
            return -1;
        }
    }
    return (reinterpret_cast<decltype(&dup3)>(libc_dup3))(oldfd, newfd, flags);
}

/* Directories related calls */

inline int dirp_to_fd(const DIR* dirp){
    assert(dirp != nullptr);
    return *(reinterpret_cast<int*>(&dirp));
}

inline DIR* fd_to_dirp(const int fd){
    assert(fd >= 0);
    return reinterpret_cast<DIR*>(fd);
}

int intcp_dirfd(DIR *dirp) {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        if(dirp == nullptr){
            errno = EINVAL;
            return -1;
        }
        auto fd = dirp_to_fd(dirp);
        if(CTX->file_map()->exist(fd)) {
            return fd;
        }
    }
    return LIBC_FUNC(dirfd, dirp);
}

DIR* opendir(const char* path){
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(opendir, path);
    }
    CTX->log()->trace("{}() called with path '{}'", __func__, path);
    std::string rel_path;
    if(!CTX->relativize_path(path, rel_path)) {
      return LIBC_FUNC(opendir, rel_path.c_str());
    }

    auto fd = adafs_opendir(rel_path);
    if(fd < 0){
        return nullptr;
    }
    return fd_to_dirp(fd);
}

DIR* fdopendir(int fd){
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        CTX->log()->trace("{}() called with fd {}", __func__, fd);
        if (CTX->file_map()->exist(fd)) {
            auto open_file = CTX->file_map()->get(fd);
            auto open_dir = static_pointer_cast<OpenDir>(open_file);
            if(!open_dir){
                //Cast did not succeeded: open_file is a regular file
                errno = EBADF;
                return nullptr;
            }
            return fd_to_dirp(fd);
        }
    }
    return LIBC_FUNC(fdopendir, fd);
}

struct dirent* intcp_readdir(DIR* dirp){
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        if(dirp == nullptr){
            errno = EINVAL;
            return nullptr;
        }
        auto fd = dirp_to_fd(dirp);
        if(CTX->file_map()->exist(fd)) {
            return adafs_readdir(fd);
        }
    }
    return LIBC_FUNC(readdir, dirp);
}

int intcp_closedir(DIR* dirp) {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        if(dirp == nullptr){
            errno = EINVAL;
            return -1;
        }
        auto fd = dirp_to_fd(dirp);
        if (CTX->file_map()->exist(fd)) {
            // No call to the daemon is required
            CTX->file_map()->remove(fd);
            return 0;
        }
    }
    return LIBC_FUNC(closedir, dirp);
}

int chmod(const char* path, mode_t mode) noexcept {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(chmod, path, mode);
    }

    CTX->log()->trace("{}() called with path '{}'", __func__, path);
    std::string rel_path;
    if (!CTX->relativize_path(path, rel_path)) {
        return LIBC_FUNC(chmod, rel_path.c_str(), mode);
    }
    CTX->log()->warn("{}() operation not supported", __func__);
    errno = ENOTSUP;
    return -1;
}

int fchmod(int fd, mode_t mode) noexcept {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        CTX->log()->trace("{}() called  [fd: {}, mode: {}]", __func__, fd, mode);
        if (CTX->file_map()->exist(fd)) {
            CTX->log()->warn("{}() operation not supported", __func__);
            errno = ENOTSUP;
            return -1;
        }
    }
    return LIBC_FUNC(fchmod, fd, mode);
}

int fchmodat(int dirfd, const char* cpath, mode_t mode, int flags) noexcept {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(fchmodat, dirfd, cpath, mode, flags);
    }

    if (cpath[0] == '\0') {
        CTX->log()->error("{}() path is invalid", __func__);
        errno = EINVAL;
        return -1;
    }

    CTX->log()->trace("{}() called path '{}', mode {}, dirfd {}, flags {}", __func__, cpath, mode, dirfd, flags);

    std::string resolved;

    if(cpath[0] != PSP) {
        if(!(CTX->file_map()->exist(dirfd))) {
            //TODO relative cpath could still lead to our FS
            return LIBC_FUNC(fchmodat, dirfd, cpath, mode, flags);
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
            CTX->log()->warn("{}() operation not supported", __func__);
            errno = ENOTSUP;
            return -1;
        }
    } else {
        if (CTX->relativize_path(cpath, resolved)) {
            CTX->log()->warn("{}() operation not supported", __func__);
            errno = ENOTSUP;
            return -1;
        }
    }
    return LIBC_FUNC(fchmodat, dirfd, resolved.c_str(), mode, flags);
}

int chdir(const char* path) noexcept {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(chdir, path);
    }

    CTX->log()->trace("{}() called with path '{}'", __func__, path);
    std::string rel_path;
    bool internal = CTX->relativize_path(path, rel_path);
    if (internal) {
        //path falls in our namespace
        struct stat st{};
        if(adafs_stat(rel_path, &st) != 0) {
            CTX->log()->error("{}() path does not exists", __func__);
            errno = ENOENT;
            return -1;
        }
        if(!S_ISDIR(st.st_mode)) {
            CTX->log()->error("{}() path is not a directory", __func__);
            errno = ENOTDIR;
            return -1;
        }
        //TODO get complete path from relativize_path instead of
        // removing mountdir and then adding again here
        rel_path.insert(0, CTX->mountdir());
        if (has_trailing_slash(rel_path)) {
            // open_dir is '/'
            rel_path.pop_back();
        }
    }
    try {
        set_cwd(rel_path, internal);
    } catch (const std::system_error& se) {
        return -1;
    }
    return 0;
}

int fchdir(int fd) noexcept {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(fchdir, fd);
    }
    CTX->log()->trace("{}() called with fd {}", __func__, fd);
    if (CTX->file_map()->exist(fd)) {
        auto open_file = CTX->file_map()->get(fd);
        auto open_dir = static_pointer_cast<OpenDir>(open_file);
        if(!open_dir){
            //Cast did not succeeded: open_file is a regular file
            CTX->log()->error("{}() file descriptor refers to a normal file: '{}'",
                    __func__, open_dir->path());
            errno = EBADF;
            return -1;
        }

        std::string new_path = CTX->mountdir() + open_dir->path();
        if (has_trailing_slash(new_path)) {
            // open_dir is '/'
            new_path.pop_back();
        }
        try {
            set_cwd(new_path, true);
        } catch (const std::system_error& se) {
            return -1;
        }
    } else {
        if(LIBC_FUNC(fchdir, fd) != 0) {
            CTX->log()->error("{}() failed to change dir: {}",
                    __func__, std::strerror(errno));
        }
        unset_env_cwd();
        CTX->cwd(get_sys_cwd());
    }
    return 0;
}

char* getcwd(char* buf, size_t size) noexcept {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(getcwd, buf, size);
    }
    CTX->log()->trace("{}() called with size {}", __func__, size);

    if(size == 0) {
        buf = static_cast<char*>(malloc(CTX->cwd().size() +  1));
        if(buf == nullptr){
            CTX->log()->error("{}() failed to allocate buffer of size {}", __func__, CTX->cwd().size());
            errno = ENOMEM;
            return nullptr;
        }
    } else if(CTX->cwd().size() + 1 > size) {
        CTX->log()->error("{}() buffer too small to host current working dir", __func__);
        errno = ERANGE;
        return nullptr;
    }

    strcpy(buf, CTX->cwd().c_str());
    return buf;
}

char* get_current_dir_name() noexcept {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return (reinterpret_cast<decltype(&get_current_dir_name)>(libc_dup3))();
    }
    CTX->log()->error("{}() not implemented", __func__);
    errno = ENOTSUP;
    return nullptr;
}


int link(const char* oldpath, const char* newpath) noexcept {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        CTX->log()->trace("{}() called [oldpath: '{}', newpath: '{}']",
               __func__, oldpath, newpath);
        CTX->log()->error("{}() not implemented", __func__);
        errno = ENOTSUP;
        return -1;
    }
    return LIBC_FUNC(link, oldpath, newpath);
}

int linkat(int olddirfd, const char *oldpath,
           int newdirfd, const char* newpath, int flags) noexcept {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        CTX->log()->trace("{}() called [olddirfd: '{}', oldpath: '{}' newdirfd: '{}', newpath: '{}', flags: '{}']",
                          __func__, olddirfd, oldpath, newdirfd, newpath, flags);
        CTX->log()->error("{}() not implemented", __func__);
        errno = ENOTSUP;
        return -1;
    }
    return LIBC_FUNC(linkat, olddirfd, oldpath, newdirfd, newpath, flags);
}

int symlink(const char* oldpath, const char* newpath) noexcept {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(symlink, oldpath, newpath);
    }
    CTX->log()->trace("{}() called [oldpath: '{}', newpath: '{}']",
            __func__, oldpath, newpath);

    std::string rel_oldpath;
    bool oldpath_internal = CTX->relativize_path(oldpath, rel_oldpath);

    std::string rel_newpath;
    bool newpath_internal = CTX->relativize_path(newpath, rel_newpath);

    if (oldpath_internal || newpath_internal) {
        CTX->log()->error("{}() not implemented", __func__);
        errno = ENOTSUP;
        return -1;
    }
    return LIBC_FUNC(symlink, rel_oldpath.c_str(), rel_newpath.c_str());
}

int symlinkat(const char* oldpath, int fd, const char* newpath) noexcept {
    init_passthrough_if_needed();
    if(CTX->initialized()) {
        CTX->log()->trace("{}() called [oldpath: '{}', newpath: '{}']",
               __func__, oldpath, newpath);
        CTX->log()->error("{}() not implemented", __func__);
        errno = ENOTSUP;
        return -1;
    }
    return LIBC_FUNC(symlinkat, oldpath, fd, newpath);
}

char *realpath(const char *path, char *resolved_path) {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(realpath, path, resolved_path);
    }

    CTX->log()->trace("{}() called with path '{}'", __func__, path);
    std::string rel_path;
    if (!CTX->relativize_path(path, rel_path)) {
        return LIBC_FUNC(realpath, rel_path.c_str(), resolved_path);
    }

    auto absolute_path = CTX->mountdir() + rel_path;
    if(absolute_path.size() >= PATH_MAX) {
        errno = ENAMETOOLONG;
        return nullptr;
    }

    if(resolved_path == nullptr) {
        resolved_path = static_cast<char*>(malloc(absolute_path.size() +  1));
        if(resolved_path == nullptr){
            CTX->log()->error("{}() failed to allocate buffer for called with path '{}'", __func__, path);
            errno = ENOMEM;
            return nullptr;
        }
    }

    strcpy(resolved_path, absolute_path.c_str());
    return resolved_path;
}

int renameat2(int olddfd, const char * oldname,
              int newdfd, const char * newname,
              unsigned int flags) {

    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(renameat2, olddfd, oldname, newdfd, newname, flags);
    }

    CTX->log()->trace("{}() called with olddfd {}, oldname: '{}', newfd {}, newname '{}', flags {}",
                      __func__, olddfd, oldname, newdfd, newname, flags);

    const char * oldpath_pass;
    std::string oldpath_resolved;
    auto oldpath_status = CTX->relativize_fd_path(olddfd, oldname, oldpath_resolved);
    switch(oldpath_status) {
        case RelativizeStatus::fd_unknown:
            oldpath_pass = oldname;
            break;

        case RelativizeStatus::external:
            oldpath_pass = oldpath_resolved.c_str();
            break;

        case RelativizeStatus::fd_not_a_dir:
            errno = ENOTDIR;
            return -1;

        case RelativizeStatus::internal:
            break;

        default:
            CTX->log()->error("{}() relativize status unknown", __func__);
            errno = EINVAL;
            return -1;
    }

    const char * newpath_pass;
    std::string newpath_resolved;
    auto newpath_status = CTX->relativize_fd_path(newdfd, newname, newpath_resolved);
    switch(newpath_status) {
        case RelativizeStatus::fd_unknown:
            newpath_pass = newname;
            break;

        case RelativizeStatus::external:
            newpath_pass = newpath_resolved.c_str();
            break;

        case RelativizeStatus::fd_not_a_dir:
            errno = ENOTDIR;
            return -1;

        case RelativizeStatus::internal:
            if (oldpath_status != RelativizeStatus::internal) {
                CTX->log()->warn("{}() cross filesystem rename not supported", __func__);
                errno = ENOTSUP;
                return -1;
            } else {
                if (flags != 0) {
                    CTX->log()->warn("{}() not supported flags", __func__);
                    errno = ENOTSUP;
                    return -1;
                }
                return adafs_rename(oldpath_resolved, newpath_resolved);
            }

        default:
            CTX->log()->error("{}() relativize status unknown", __func__);
            errno = EINVAL;
            return -1;
    }

    assert(newpath_status == RelativizeStatus::external);

    /* newpath is internal but oldpath is external */
    if (oldpath_status == RelativizeStatus::internal) {
        CTX->log()->warn("{}() cross filesystem rename not supported", __func__);
        errno = ENOTSUP;
        return -1;
    }

    return LIBC_FUNC(renameat2, olddfd, oldpath_pass, newdfd, newpath_pass, flags);
}

int rename(const char *oldpath, const char *newpath) {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(rename, oldpath, newpath);
    }
    return renameat2(AT_FDCWD, oldpath, AT_FDCWD, newpath, 0);
}

int __renameat(int olddfd, const char * oldname,
             int newdfd, const char * newname) {
    init_passthrough_if_needed();
    if(!CTX->initialized()) {
        return LIBC_FUNC(__renameat, olddfd, oldname, newdfd, newname);
    }
    return renameat2(olddfd, oldname, newdfd, newname, 0);
}


