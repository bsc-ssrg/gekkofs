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
    CTX->log()->debug("Deactivating interception of syscalls");
#endif
    // Reset callback function pointer
    intercept_hook_point = nullptr;
}
