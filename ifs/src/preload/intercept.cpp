#include "preload/intercept.hpp"
#include "preload/preload.hpp"
#include "preload/hooks.hpp"

#ifndef NDEBUG
#include "preload/syscall_names.hpp"
#endif

#include <libsyscall_intercept_hook_point.h>
#include <syscall.h>
#include <errno.h>


#define NOT_HOOKED 1
#define HOOKED 0


static inline int hook(long syscall_number,
         long arg0, long arg1, long arg2,
         long arg3, long arg4, long arg5,
         long *result)
{

    switch (syscall_number) {

    case SYS_open:
        *result = hook_openat(AT_FDCWD,
                              reinterpret_cast<char*>(arg0),
                              static_cast<int>(arg1),
                              static_cast<mode_t>(arg2));
        break;

    case SYS_openat:
        *result = hook_openat(static_cast<int>(arg0),
                              reinterpret_cast<const char*>(arg1),
                              static_cast<int>(arg2),
                              static_cast<mode_t>(arg3));
        break;

    case SYS_close:
        *result = hook_close(static_cast<int>(arg0));
        break;

    case SYS_stat:
        *result = hook_stat(reinterpret_cast<char*>(arg0),
                            reinterpret_cast<struct stat*>(arg1));
        break;

    case SYS_lstat:
        *result = hook_lstat(reinterpret_cast<char*>(arg0),
                             reinterpret_cast<struct stat*>(arg1));
        break;

    case SYS_fstat:
        *result = hook_fstat(static_cast<int>(arg0),
                            reinterpret_cast<struct stat*>(arg1));
        break;

    case SYS_read:
        *result = hook_read(static_cast<unsigned int>(arg0),
                            reinterpret_cast<void*>(arg1),
                            static_cast<size_t>(arg2));
        break;

    case SYS_write:
        *result = hook_write(static_cast<unsigned int>(arg0),
                             reinterpret_cast<void*>(arg1),
                             static_cast<size_t>(arg2));
        break;

    case SYS_writev:
        *result = hook_writev(static_cast<unsigned long>(arg0),
                              reinterpret_cast<const struct iovec *>(arg1),
                              static_cast<unsigned long>(arg2));
        break;

    case SYS_unlink:
        *result = hook_unlink(reinterpret_cast<char*>(arg0));
        break;

    case SYS_access:
        *result = hook_access(reinterpret_cast<char*>(arg0),
                              static_cast<int>(arg1));
        break;

    case SYS_lseek:
        *result = hook_lseek(static_cast<unsigned int>(arg0),
                             static_cast<off_t>(arg1),
                             static_cast<unsigned int>(arg2));
        break;

    case SYS_dup:
        *result = hook_dup(static_cast<unsigned int>(arg0));
        break;

    case SYS_dup2:
        *result = hook_dup2(static_cast<unsigned int>(arg0),
                            static_cast<unsigned int>(arg1));
        break;

    case SYS_dup3:
        *result = hook_dup3(static_cast<unsigned int>(arg0),
                            static_cast<unsigned int>(arg1),
                            static_cast<int>(arg2));
        break;

    case SYS_getdents:
        *result = hook_getdents(static_cast<unsigned int>(arg0),
                                reinterpret_cast<struct linux_dirent *>(arg1),
                                static_cast<unsigned int>(arg2));
        break;

    case SYS_mkdirat:
        *result = hook_mkdirat(static_cast<unsigned int>(arg0),
                               reinterpret_cast<const char *>(arg1),
                               static_cast<mode_t>(arg2));
        break;

    case SYS_mkdir:
        *result = hook_mkdirat(AT_FDCWD,
                               reinterpret_cast<const char *>(arg0),
                               static_cast<mode_t>(arg1));
        break;

    case SYS_chdir:
        *result = hook_chdir(reinterpret_cast<const char *>(arg0));
        break;

    case SYS_fchdir:
        *result = hook_fchdir(static_cast<unsigned int>(arg0));
        break;

    case SYS_getcwd:
        *result = hook_getcwd(reinterpret_cast<char *>(arg0),
                               static_cast<unsigned long>(arg1));
        break;

    default:
        /*
         * Ignore any other syscalls
         * i.e.: pass them on to the kernel
         * as would normally happen.
         */

        #ifndef NDEBUG
        CTX->log()->trace("Syscall [{}, {}]  Passthrough", syscall_names[syscall_number], syscall_number);
        #endif
        return NOT_HOOKED;
    }

    #ifndef NDEBUG
    CTX->log()->trace("Syscall [{}, {}]  Intercepted", syscall_names[syscall_number], syscall_number);
    #endif
    return HOOKED;
}


static __thread bool guard_flag;

int
hook_guard_wrapper(long syscall_number,
                   long arg0, long arg1, long arg2,
                   long arg3, long arg4, long arg5,
                   long *syscall_return_value)
{
    assert(CTX->initialized());

    if (guard_flag) {
        return NOT_HOOKED;
    }

    int is_hooked;

    guard_flag = true;
    int oerrno = errno;
    is_hooked = hook(syscall_number,
                     arg0, arg1, arg2, arg3, arg4, arg5,
                     syscall_return_value);
    errno = oerrno;
    guard_flag = false;

    return is_hooked;
}


void start_interception() {
    assert(CTX->initialized());
#ifndef NDEBUG
    CTX->log()->debug("Activating interception of syscalls");
#endif
    // Set up the callback function pointer
    intercept_hook_point = hook_guard_wrapper;
}

void stop_interception() {
    assert(CTX->initialized());
#ifndef NDEBUG
    //CTX->log()->debug("Deactivating interception of syscalls");
#endif
    // Reset callback function pointer
    intercept_hook_point = nullptr;
}
