#ifndef IFS_INTERCEPT_HPP
#define IFS_INTERCEPT_HPP

int
hook_guard_wrapper(long syscall_number,
                   long arg0, long arg1, long arg2,
                   long arg3, long arg4, long arg5,
                   long *syscall_return_value);

void start_interception();
void stop_interception();

#endif
