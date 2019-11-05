/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#define _GNU_SOURCE
#include <syscall.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <client/syscalls/detail/syscall_info.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define SYSCALL(id, nargs, ret, ...) \
    [SYS_##id] =                     \
{                                    \
    .s_nr = SYS_##id,                \
    .s_name = #id,                   \
    .s_nargs = nargs,                \
    .s_return_type = ret,            \
    .s_args = {__VA_ARGS__}          \
}

#define S_NOARGS() {0}

#define S_UARG(t) \
{                 \
    .a_type = t,  \
    .a_name = #t  \
}

#define S_NARG(t, n) \
{                    \
    .a_type = t,     \
    .a_name = n      \
}

#define S_RET(t) \
{                \
    .r_type = t  \
}

/* Linux syscalls on x86_64 */
const struct syscall_info syscall_table[] = {
    SYSCALL(read,                    3,  S_RET(rdec),    S_UARG(fd),                    S_NARG(ptr, "buf"),              S_NARG(arg, "count")),          
    SYSCALL(write,                   3,  S_RET(rdec),    S_UARG(fd),                    S_NARG(ptr, "buf"),              S_NARG(arg, "count")),
    SYSCALL(open,                    2,  S_RET(rdec),    S_NARG(cstr, "pathname"),      S_NARG(open_flags, "flags")),
    SYSCALL(close,                   1,  S_RET(rdec),    S_UARG(fd)),
    SYSCALL(stat,                    2,  S_RET(rdec),    S_NARG(cstr, "pathname"),      S_NARG(ptr, "statbuf")),
    SYSCALL(fstat,                   2,  S_RET(rdec),    S_UARG(fd),                    S_NARG(ptr, "statbuf")),
    SYSCALL(lstat,                   2,  S_RET(rdec),    S_NARG(cstr, "pathname"),      S_NARG(ptr, "statbuf")),
    SYSCALL(poll,                    3,  S_RET(rdec),    S_NARG(ptr, "fds"),            S_NARG(dec, "nfds"),             S_NARG(dec, "timeout")),
    SYSCALL(lseek,                   3,  S_RET(rdec),    S_UARG(fd),                    S_UARG(offset),                  S_UARG(whence)),
    SYSCALL(mmap,                    6,  S_RET(rptr),    S_NARG(ptr, "addr"),           S_NARG(dec, "length"),           S_NARG(mmap_prot, "prot"),        S_NARG(mmap_flags, "flags"), S_UARG(fd),                  S_UARG(offset)),
    SYSCALL(mprotect,                3,  S_RET(rdec),    S_NARG(ptr, "addr"),           S_NARG(dec, "length"),           S_NARG(mmap_prot, "prot")),
    SYSCALL(munmap,                  2,  S_RET(rdec),    S_NARG(ptr, "addr"),           S_NARG(dec, "length")),
    SYSCALL(brk,                     1,  S_RET(rdec),    S_NARG(ptr, "addr")),
    SYSCALL(rt_sigaction,            4,  S_RET(rdec),    S_NARG(signum, "signum"),      S_NARG(ptr, "act"),              S_NARG(ptr, "oldact"),            S_NARG(dec, "sigsetsize")),
    SYSCALL(rt_sigprocmask,          4,  S_RET(rdec),    S_NARG(sigproc_how, "how"),    S_NARG(ptr, "set"),              S_NARG(ptr, "oldset"),            S_NARG(dec, "sigsetsize")),
    SYSCALL(rt_sigreturn,            0,  S_RET(rnone),   S_NOARGS()),
    SYSCALL(ioctl,                   3,  S_RET(rdec),    S_UARG(fd),                    S_NARG(arg, "cmd"),              S_NARG(arg, "argp")),
    SYSCALL(pread64,                 4,  S_RET(rdec),    S_UARG(fd),                    S_NARG(ptr, "buf"),              S_NARG(arg, "count"),             S_UARG(offset)),
    SYSCALL(pwrite64,                4,  S_RET(rdec),    S_UARG(fd),                    S_NARG(ptr, "buf"),              S_NARG(arg, "count"),             S_UARG(offset)),
    SYSCALL(readv,                   3,  S_RET(rdec),    S_UARG(fd),                    S_NARG(ptr, "iov"),              S_NARG(dec, "iovcnt")),
    SYSCALL(writev,                  3,  S_RET(rdec),    S_UARG(fd),                    S_NARG(ptr, "iov"),              S_NARG(dec, "iovcnt")),
    SYSCALL(access,                  2,  S_RET(rdec),    S_NARG(cstr, "pathname"),      S_NARG(octal_mode, "mode")),
    SYSCALL(pipe,                    1,  S_RET(rdec),    S_NARG(ptr, "pipefd")),
    SYSCALL(select,                  5,  S_RET(rdec),    S_NARG(dec, "nfds"),           S_NARG(ptr, "readfds"),          S_NARG(ptr, "writefds"),          S_NARG(ptr, "exceptfds"),    S_NARG(ptr, "timeout")),
    SYSCALL(sched_yield,             0,  S_RET(rdec),    S_NOARGS()),
    SYSCALL(mremap,                  5,  S_RET(rdec),    S_NARG(ptr, "old_address"),    S_NARG(dec, "old_size"),         S_NARG(dec, "new_size"),          S_NARG(arg, "flags"),        S_NARG(ptr, "new_addr")),
    SYSCALL(msync,                   3,  S_RET(rdec),    S_NARG(ptr, "addr"),           S_NARG(dec, "length"),           S_NARG(arg, "flags")),
    SYSCALL(mincore,                 3,  S_RET(rdec),    S_NARG(ptr, "addr"),           S_NARG(dec, "length"),           S_NARG(ptr, "vec")),
    SYSCALL(madvise,                 3,  S_RET(rdec),    S_NARG(ptr, "addr"),           S_NARG(dec, "length"),           S_NARG(arg, "behavior")),
    SYSCALL(shmget,                  3,  S_RET(rdec),    S_NARG(arg, "key"),            S_NARG(dec, "size"),             S_NARG(arg, "flag")),
    SYSCALL(shmat,                   3,  S_RET(rdec),    S_NARG(arg, "shmid"),          S_NARG(ptr, "shmaddr"),          S_NARG(arg, "shmflg")),
    SYSCALL(shmctl,                  3,  S_RET(rdec),    S_NARG(arg, "shmid"),          S_NARG(arg, "cmd"),              S_NARG(ptr, "buf")),
    SYSCALL(dup,                     1,  S_RET(rdec),    S_NARG(fd, "oldfd")),
    SYSCALL(dup2,                    2,  S_RET(rdec),    S_NARG(fd, "oldfd"),           S_NARG(fd, "newfd")),
    SYSCALL(pause,                   0,  S_RET(rdec),    S_NOARGS()),
    SYSCALL(nanosleep,               2,  S_RET(rdec),    S_NARG(ptr, "rqtp"),           S_NARG(ptr, "rmtp")),
    SYSCALL(getitimer,               2,  S_RET(rdec),    S_NARG(arg, "which"),          S_NARG(ptr, "value")),
    SYSCALL(alarm,                   1,  S_RET(rdec),    S_NARG(dec, "seconds")),
    SYSCALL(setitimer,               3,  S_RET(rdec),    S_NARG(arg, "which"),          S_NARG(ptr, "value"),            S_NARG(ptr, "ovalue")),
    SYSCALL(getpid,                  0,  S_RET(rdec),    S_NOARGS()),
    SYSCALL(sendfile,                4,  S_RET(rdec),    S_NARG(fd, "out_fd"),          S_NARG(fd, "in_fd"),             S_NARG(ptr, "offset"),            S_NARG(arg, "count")),
    SYSCALL(socket,                  3,  S_RET(rdec),    S_NARG(arg, "domain"),         S_NARG(arg, "type"),             S_NARG(arg, "protocol")),
    SYSCALL(connect,                 3,  S_RET(rdec),    S_NARG(fd, "sockfd"),          S_NARG(ptr, "addr"),             S_NARG(arg, "addrlen")),
    SYSCALL(accept,                  3,  S_RET(rdec),    S_NARG(fd, "sockfd"),          S_NARG(ptr, "addr"),             S_NARG(ptr, "addrlen")),
    SYSCALL(sendto,                  5,  S_RET(rdec),    S_NARG(fd, "sockfd"),          S_NARG(ptr, "dest_addr"),        S_NARG(arg, "len"),               S_NARG(ptr, "addr"),         S_NARG(arg, "addrlen")),
    SYSCALL(recvfrom,                5,  S_RET(rdec),    S_NARG(fd, "sockfd"),          S_NARG(ptr, "src_addr"),         S_NARG(arg, "len"),               S_NARG(ptr, "addr"),         S_NARG(ptr, "addrlen")),
    SYSCALL(sendmsg,                 3,  S_RET(rdec),    S_NARG(fd, "sockfd"),          S_NARG(ptr, "msg"),              S_NARG(arg, "flags")),
    SYSCALL(recvmsg,                 3,  S_RET(rdec),    S_NARG(fd, "sockfd"),          S_NARG(ptr, "msg"),              S_NARG(arg, "flags")),
    SYSCALL(shutdown,                2,  S_RET(rdec),    S_NARG(fd, "sockfd"),          S_NARG(arg, "how")),
    SYSCALL(bind,                    3,  S_RET(rdec),    S_NARG(fd, "sockfd"),          S_NARG(ptr, "addr"),             S_NARG(arg, "addrlen")),
    SYSCALL(listen,                  2,  S_RET(rdec),    S_NARG(fd, "sockfd"),          S_NARG(arg, "backlog")),
    SYSCALL(getsockname,             3,  S_RET(rdec),    S_NARG(fd, "sockfd"),          S_NARG(ptr, "addr"),             S_NARG(ptr, "addrlen")),
    SYSCALL(getpeername,             3,  S_RET(rdec),    S_NARG(fd, "sockfd"),          S_NARG(ptr, "addr"),             S_NARG(ptr, "addrlen")),
    SYSCALL(socketpair,              4,  S_RET(rdec),    S_NARG(arg, "domain"),         S_NARG(arg, "type"),             S_NARG(arg, "protocol"),          S_NARG(ptr, "sv")),
    SYSCALL(setsockopt,              5,  S_RET(rdec),    S_UARG(fd),                    S_NARG(arg, "level"),            S_NARG(arg, "optname"),           S_NARG(ptr, "optval"),       S_NARG(arg, "optlen")),
    SYSCALL(getsockopt,              5,  S_RET(rdec),    S_UARG(fd),                    S_NARG(arg, "level"),            S_NARG(arg, "optname"),           S_NARG(ptr, "optval"),       S_NARG(ptr, "optlen")),
    SYSCALL(clone,                   5,  S_RET(rdec),    S_NARG(clone_flags, "flags"),  S_NARG(ptr, "child_stack"),      S_NARG(ptr, "ptid"),              S_NARG(ptr, "ctid"),         S_NARG(ptr, "newtls")),
    SYSCALL(fork,                    0,  S_RET(rdec),    S_NOARGS()),
    SYSCALL(vfork,                   0,  S_RET(rdec),    S_NOARGS()),
    SYSCALL(execve,                  3,  S_RET(rdec),    S_NARG(cstr, "pathname"),      S_NARG(ptr, "argv"),             S_NARG(ptr, "envp")),
    SYSCALL(exit,                    1,  S_RET(rnone),   S_NARG(dec, "status")),
    SYSCALL(wait4,                   4,  S_RET(rdec),    S_NARG(dec, "pid"),            S_NARG(ptr, "stat_addr"),        S_NARG(arg, "options"),           S_NARG(ptr, "rusage")),
    SYSCALL(kill,                    2,  S_RET(rdec),    S_NARG(dec, "pid"),            S_NARG(signum, "sig")),
    SYSCALL(uname,                   1,  S_RET(rdec),    S_NARG(ptr, "buf")),
    SYSCALL(semget,                  3,  S_RET(rdec),    S_NARG(arg, "key"),            S_NARG(dec, "nsems"),            S_NARG(arg, "semflg")),
    SYSCALL(semop,                   3,  S_RET(rdec),    S_NARG(dec, "semid"),          S_NARG(ptr, "sops"),             S_NARG(arg, "nsops")),
    SYSCALL(semctl,                  4,  S_RET(rdec),    S_NARG(dec, "semid"),          S_NARG(dec, "semnum"),           S_NARG(arg, "cmd"),               S_NARG(arg, "arg")),
    SYSCALL(shmdt,                   1,  S_RET(rdec),    S_NARG(ptr, "shmaddr")),
    SYSCALL(msgget,                  2,  S_RET(rdec),    S_NARG(arg, "key"),            S_NARG(arg, "msflg")),
    SYSCALL(msgsnd,                  4,  S_RET(rdec),    S_NARG(arg, "msqid"),          S_NARG(ptr, "msgp"),             S_NARG(dec, "msgsz"),             S_NARG(arg, "msflg")),
    SYSCALL(msgrcv,                  5,  S_RET(rdec),    S_NARG(arg, "msqid"),          S_NARG(ptr, "msgp"),             S_NARG(dec, "msgsz"),             S_NARG(arg, "msgtyp"),       S_NARG(arg, "msflg")),
    SYSCALL(msgctl,                  3,  S_RET(rdec),    S_NARG(arg, "msqid"),          S_NARG(arg, "cmd"),              S_NARG(ptr, "buf")),
    SYSCALL(fcntl,                   3,  S_RET(rdec),    S_UARG(fd),                    S_NARG(arg, "cmd"),              S_NARG(arg, "arg")),
    SYSCALL(flock,                   2,  S_RET(rdec),    S_UARG(fd),                    S_NARG(arg, "cmd")),
    SYSCALL(fsync,                   1,  S_RET(rdec),    S_UARG(fd)),
    SYSCALL(fdatasync,               2,  S_RET(rdec),    S_UARG(fd)),
    SYSCALL(truncate,                2,  S_RET(rdec),    S_NARG(cstr, "pathname"),      S_NARG(arg, "length")),
    SYSCALL(ftruncate,               2,  S_RET(rdec),    S_UARG(fd),                    S_NARG(offset, "length")),
    SYSCALL(getdents,                3,  S_RET(rdec),    S_UARG(fd),                    S_NARG(ptr, "dirent"),           S_NARG(arg, "count")),
    SYSCALL(getcwd,                  2,  S_RET(rdec),    S_NARG(ptr, "buf"),            S_NARG(dec, "size")),
    SYSCALL(chdir,                   1,  S_RET(rdec),    S_NARG(cstr, "pathname")),
    SYSCALL(fchdir,                  1,  S_RET(rdec),    S_UARG(fd)),
    SYSCALL(rename,                  2,  S_RET(rdec),    S_NARG(cstr, "oldpath"),       S_NARG(cstr, "newpath")),
    SYSCALL(mkdir,                   2,  S_RET(rdec),    S_NARG(cstr, "pathname"),      S_NARG(octal_mode, "mode")),
    SYSCALL(rmdir,                   1,  S_RET(rdec),    S_NARG(cstr, "pathname")),
    SYSCALL(creat,                   2,  S_RET(rdec),    S_NARG(cstr, "pathname"),      S_NARG(octal_mode, "mode")),
    SYSCALL(link,                    2,  S_RET(rdec),    S_NARG(cstr, "oldpath"),       S_NARG(cstr, "newpath")),
    SYSCALL(unlink,                  1,  S_RET(rdec),    S_NARG(cstr, "pathname")),
    SYSCALL(symlink,                 2,  S_RET(rdec),    S_NARG(cstr, "target"),        S_NARG(cstr, "linkpath")),
    SYSCALL(readlink,                2,  S_RET(rdec),    S_NARG(cstr, "pathname"),      S_NARG(ptr, "buf"),              S_NARG(arg, "bufsiz")),
    SYSCALL(chmod,                   2,  S_RET(rdec),    S_NARG(cstr, "pathname"),      S_NARG(octal_mode, "mode")),
    SYSCALL(fchmod,                  2,  S_RET(rdec),    S_UARG(fd),                    S_NARG(octal_mode, "mode")),
    SYSCALL(chown,                   3,  S_RET(rdec),    S_NARG(cstr, "pathname"),      S_NARG(dec, "user"),             S_NARG(dec, "group")),
    SYSCALL(fchown,                  3,  S_RET(rdec),    S_UARG(fd),                    S_NARG(dec, "user"),             S_NARG(dec, "group")),
    SYSCALL(lchown,                  3,  S_RET(rdec),    S_NARG(cstr, "pathname"),      S_NARG(dec, "user"),             S_NARG(dec, "group")),
    SYSCALL(umask,                   1,  S_RET(rdec),    S_NARG(arg, "mask")),
    SYSCALL(gettimeofday,            2,  S_RET(rdec),    S_NARG(ptr, "tv"),             S_NARG(ptr, "tz")),
    SYSCALL(getrlimit,               2,  S_RET(rdec),    S_NARG(arg, "resource"),       S_NARG(ptr, "rlim")),
    SYSCALL(getrusage,               2,  S_RET(rdec),    S_NARG(arg, "who"),            S_NARG(ptr, "ru")),
    SYSCALL(sysinfo,                 1,  S_RET(rdec),    S_NARG(ptr, "info")),
    SYSCALL(times,                   1,  S_RET(rdec),    S_NARG(ptr, "tbuf")),
    SYSCALL(ptrace,                  4,  S_RET(rdec),    S_NARG(arg, "request"),        S_NARG(dec, "pid"),              S_NARG(ptr, "addr"),              S_NARG(ptr, "data")),
    SYSCALL(getuid,                  0,  S_RET(rdec),    S_NOARGS()),
    SYSCALL(syslog,                  3,  S_RET(rdec),    S_NARG(arg, "type"),           S_NARG(ptr, "buf"),              S_NARG(arg, "length")),
    SYSCALL(getgid,                  0,  S_RET(rdec),    S_NOARGS()),
    SYSCALL(setuid,                  1,  S_RET(rdec),    S_NARG(dec, "uid")),
    SYSCALL(setgid,                  1,  S_RET(rdec),    S_NARG(dec, "gid")),
    SYSCALL(geteuid,                 0,  S_RET(rdec),    S_NOARGS()),
    SYSCALL(getegid,                 0,  S_RET(rdec),    S_NOARGS()),
    SYSCALL(setpgid,                 2,  S_RET(rdec),    S_NARG(dec, "pid"),            S_NARG(dec, "pgid")),
    SYSCALL(getppid,                 0,  S_RET(rdec),    S_NOARGS()),
    SYSCALL(getpgrp,                 0,  S_RET(rdec),    S_NOARGS()),
    SYSCALL(setsid,                  0,  S_RET(rdec),    S_NOARGS()),
    SYSCALL(setreuid,                2,  S_RET(rdec),    S_NARG(dec, "ruid"),           S_NARG(dec, "euid")),
    SYSCALL(setregid,                2,  S_RET(rdec),    S_NARG(dec, "rgid"),           S_NARG(dec, "egid")),
    SYSCALL(getgroups,               2,  S_RET(rdec),    S_NARG(arg, "gidsetsize"),     S_NARG(ptr, "grouplist")),
    SYSCALL(setgroups,               2,  S_RET(rdec),    S_NARG(arg, "gidsetsize"),     S_NARG(ptr, "grouplist")),
    SYSCALL(setresuid,               3,  S_RET(rdec),    S_NARG(dec, "ruid"),           S_NARG(dec, "euid"),             S_NARG(dec, "suid")),
    SYSCALL(getresuid,               3,  S_RET(rdec),    S_NARG(ptr, "ruid"),           S_NARG(ptr, "euid"),             S_NARG(ptr, "suid")),
    SYSCALL(setresgid,               3,  S_RET(rdec),    S_NARG(dec, "rgid"),           S_NARG(dec, "egid"),             S_NARG(dec, "sgid")),
    SYSCALL(getresgid,               3,  S_RET(rdec),    S_NARG(ptr, "rgid"),           S_NARG(ptr, "egid"),             S_NARG(ptr, "sgid")),
    SYSCALL(getpgid,                 1,  S_RET(rdec),    S_NARG(dec, "pid")),
    SYSCALL(setfsuid,                1,  S_RET(rdec),    S_NARG(dec, "uid")),
    SYSCALL(setfsgid,                1,  S_RET(rdec),    S_NARG(dec, "gid")),
    SYSCALL(getsid,                  1,  S_RET(rdec),    S_NARG(dec, "pid")),
    SYSCALL(capget,                  2,  S_RET(rdec),    S_NARG(ptr, "header"),         S_NARG(ptr, "datap")),
    SYSCALL(capset,                  2,  S_RET(rdec),    S_NARG(ptr, "header"),         S_NARG(ptr, "datap")),
    SYSCALL(rt_sigpending,           2,  S_RET(rdec),    S_NARG(ptr, "set"),            S_NARG(dec, "sigsetsize")),
    SYSCALL(rt_sigtimedwait,         4,  S_RET(rdec),    S_NARG(ptr, "uthese"),         S_NARG(ptr, "uinfo"),            S_NARG(ptr, "uts"),               S_NARG(dec, "sigsetsize")),
    SYSCALL(rt_sigqueueinfo,         4,  S_RET(rdec),    S_NARG(dec, "pid"),            S_NARG(signum, "sig"),           S_NARG(ptr, "uinfo")),
    SYSCALL(rt_sigsuspend,           2,  S_RET(rdec),    S_NARG(ptr, "unewset"),        S_NARG(dec, "sigsetsize")),
    SYSCALL(sigaltstack,             2,  S_RET(rdec),    S_NARG(ptr, "ss"),             S_NARG(ptr, "old_ss")),
    SYSCALL(utime,                   2,  S_RET(rdec),    S_NARG(cstr, "filename"),      S_NARG(ptr, "times")),
    SYSCALL(mknod,                   3,  S_RET(rdec),    S_NARG(cstr, "pathname"),      S_NARG(octal_mode, "mode"),      S_NARG(arg, "dev")),
    SYSCALL(uselib,                  1,  S_RET(rdec),    S_NARG(cstr, "library")),
    SYSCALL(personality,             1,  S_RET(rdec),    S_NARG(arg, "personality")),
    SYSCALL(ustat,                   2,  S_RET(rdec),    S_NARG(arg, "dev"),            S_NARG(ptr, "ubuf")),
    SYSCALL(statfs,                  2,  S_RET(rdec),    S_NARG(cstr, "path"),          S_NARG(ptr, "buf")),
    SYSCALL(fstatfs,                 2,  S_RET(rdec),    S_UARG(fd),                    S_NARG(ptr, "buf")),
    SYSCALL(sysfs,                   3,  S_RET(rdec),    S_NARG(arg, "option"),         S_NARG(ptr, "arg1"),             S_NARG(ptr, "arg2")),
    SYSCALL(getpriority,             2,  S_RET(rdec),    S_NARG(arg, "which"),          S_NARG(arg, "who")),
    SYSCALL(setpriority,             3,  S_RET(rdec),    S_NARG(arg, "which"),          S_NARG(arg, "who"),              S_NARG(arg, "prio")),
    SYSCALL(sched_setparam,          2,  S_RET(rdec),    S_NARG(dec, "pid"),            S_NARG(ptr, "param")),
    SYSCALL(sched_getparam,          2,  S_RET(rdec),    S_NARG(dec, "pid"),            S_NARG(ptr, "param")),
    SYSCALL(sched_setscheduler,      3,  S_RET(rdec),    S_NARG(dec, "pid"),            S_NARG(arg, "policy"),           S_NARG(ptr, "param")),
    SYSCALL(sched_getscheduler,      1,  S_RET(rdec),    S_NARG(dec, "pid")),
    SYSCALL(sched_get_priority_max,  1,  S_RET(rdec),    S_NARG(arg, "policy")),
    SYSCALL(sched_get_priority_min,  1,  S_RET(rdec),    S_NARG(arg, "policy")),
    SYSCALL(sched_rr_get_interval,   2,  S_RET(rdec),    S_NARG(dec, "pid"),            S_NARG(ptr, "interval")),
    SYSCALL(mlock,                   2,  S_RET(rdec),    S_NARG(ptr, "addr"),           S_NARG(dec, "length")),
    SYSCALL(munlock,                 2,  S_RET(rdec),    S_NARG(ptr, "addr"),           S_NARG(dec, "length")),
    SYSCALL(mlockall,                1,  S_RET(rdec),    S_NARG(arg, "flags")),
    SYSCALL(munlockall,              0,  S_RET(rdec),    S_NOARGS()),
    SYSCALL(vhangup,                 0,  S_RET(rdec),    S_NOARGS()),
    SYSCALL(modify_ldt,              3,  S_RET(rdec),    S_NARG(arg, "func"),           S_NARG(ptr, "ptr"),              S_NARG(arg, "bytecount")),
    SYSCALL(pivot_root,              2,  S_RET(rdec),    S_NARG(cstr, "new_root"),      S_NARG(cstr, "put_old")),
    SYSCALL(_sysctl,                 1,  S_RET(rdec),    S_NARG(ptr, "args")),
    SYSCALL(prctl,                   5,  S_RET(rdec),    S_NARG(arg, "option"),         S_NARG(arg, "arg2"),             S_NARG(arg, "arg3"),              S_NARG(arg, "arg4"),         S_NARG(arg, "arg5")),
    SYSCALL(arch_prctl,              2,  S_RET(rdec),    S_NARG(arg, "code"),           S_NARG(arg, "addr")),
    SYSCALL(adjtimex,                1,  S_RET(rdec),    S_NARG(ptr, "txc_p")),
    SYSCALL(setrlimit,               2,  S_RET(rdec),    S_NARG(arg, "resource"),       S_NARG(ptr, "rlim")),
    SYSCALL(chroot,                  1,  S_RET(rdec),    S_NARG(cstr, "pathname")),
    SYSCALL(sync,                    0,  S_RET(rdec),    S_NOARGS()),
    SYSCALL(acct,                    1,  S_RET(rdec),    S_NARG(cstr, "pathname")),
    SYSCALL(settimeofday,            2,  S_RET(rdec),    S_NARG(ptr, "tv"),             S_NARG(ptr, "tz")),
    SYSCALL(mount,                   5,  S_RET(rdec),    S_NARG(cstr, "dev_name"),      S_NARG(cstr, "dir_name"),        S_NARG(cstr, "type"),             S_NARG(arg, "flags"),        S_NARG(ptr, "data")),
    SYSCALL(umount2,                 2,  S_RET(rdec),    S_NARG(cstr, "target"),        S_NARG(arg, "flags")),
    SYSCALL(swapon,                  2,  S_RET(rdec),    S_NARG(cstr, "pathname"),      S_NARG(arg, "swap_flags")),
    SYSCALL(swapoff,                 1,  S_RET(rdec),    S_NARG(cstr, "pathname")),
    SYSCALL(reboot,                  4,  S_RET(rdec),    S_NARG(arg, "magic1"),         S_NARG(arg, "magic2"),           S_NARG(arg, "cmd"),               S_NARG(ptr, "arg")),
    SYSCALL(sethostname,             2,  S_RET(rdec),    S_NARG(cstr, "name"),          S_NARG(arg, "length")),
    SYSCALL(setdomainname,           2,  S_RET(rdec),    S_NARG(cstr, "name"),          S_NARG(arg, "length")),
    SYSCALL(iopl,                    1,  S_RET(rdec),    S_NARG(arg, "level")),
    SYSCALL(ioperm,                  3,  S_RET(rdec),    S_NARG(arg, "from"),           S_NARG(arg, "num"),              S_NARG(arg, "on")),
    SYSCALL(create_module,           2,  S_RET(rdec),    S_NARG(cstr, "name"),          S_NARG(arg, "size")),
    SYSCALL(init_module,             3,  S_RET(rdec),    S_NARG(ptr, "module_image"),   S_NARG(dec, "length"),           S_NARG(cstr, "param_values")),
    SYSCALL(delete_module,           2,  S_RET(rdec),    S_NARG(cstr, "name"),          S_NARG(arg, "flags")),
    SYSCALL(get_kernel_syms,         1,  S_RET(rdec),    S_NARG(ptr, "table")),
    SYSCALL(query_module,            5,  S_RET(rdec),    S_NARG(cstr, "name"),          S_NARG(arg, "which"),            S_NARG(ptr, "buf"),               S_NARG(arg, "bufsize"),      S_NARG(ptr, "ret")),
    SYSCALL(quotactl,                4,  S_RET(rdec),    S_NARG(arg, "cmd"),            S_NARG(cstr, "special"),         S_NARG(arg, "id"),                S_NARG(ptr, "addr")),
    SYSCALL(nfsservctl,              3,  S_RET(rdec),    S_NARG(arg, "cmd"),            S_NARG(ptr, "argp"),             S_NARG(ptr, "resp")),
    SYSCALL(getpmsg,                 5,  S_RET(rdec),    S_NARG(arg, "arg0"),           S_NARG(arg, "arg1"),             S_NARG(arg, "arg2"),              S_NARG(arg, "arg3"),         S_NARG(arg, "arg4")),
    SYSCALL(putpmsg,                 5,  S_RET(rdec),    S_NARG(arg, "arg0"),           S_NARG(arg, "arg1"),             S_NARG(arg, "arg2"),              S_NARG(arg, "arg3"),         S_NARG(arg, "arg4")),
    SYSCALL(afs_syscall,             5,  S_RET(rdec),    S_NARG(arg, "arg0"),           S_NARG(arg, "arg1"),             S_NARG(arg, "arg2"),              S_NARG(arg, "arg3"),         S_NARG(arg, "arg4")),
    SYSCALL(tuxcall,                 3,  S_RET(rdec),    S_NARG(arg, "arg0"),           S_NARG(arg, "arg1"),             S_NARG(arg, "arg2")),
    SYSCALL(security,                3,  S_RET(rdec),    S_NARG(arg, "arg0"),           S_NARG(arg, "arg1"),             S_NARG(arg, "arg2")),
    SYSCALL(gettid,                  0,  S_RET(rdec),    S_NOARGS()),
    SYSCALL(readahead,               3,  S_RET(rdec),    S_UARG(fd),                    S_UARG(offset),                  S_NARG(arg, "count")),
    SYSCALL(setxattr,                5,  S_RET(rdec),    S_NARG(cstr, "pathname"),      S_NARG(cstr, "pathname"),        S_NARG(ptr, "value"),             S_NARG(dec, "size"),         S_NARG(arg, "flags")),
    SYSCALL(lsetxattr,               5,  S_RET(rdec),    S_NARG(cstr, "pathname"),      S_NARG(cstr, "pathname"),        S_NARG(ptr, "value"),             S_NARG(dec, "size"),         S_NARG(arg, "flags")),
    SYSCALL(fsetxattr,               5,  S_RET(rdec),    S_UARG(fd),                    S_NARG(cstr, "pathname"),        S_NARG(ptr, "value"),             S_NARG(dec, "size"),         S_NARG(arg, "flags")),
    SYSCALL(getxattr,                4,  S_RET(rdec),    S_NARG(cstr, "pathname"),      S_NARG(cstr, "pathname"),        S_NARG(ptr, "value"),             S_NARG(dec, "size")),
    SYSCALL(lgetxattr,               4,  S_RET(rdec),    S_NARG(cstr, "pathname"),      S_NARG(cstr, "pathname"),        S_NARG(ptr, "value"),             S_NARG(dec, "size")),
    SYSCALL(fgetxattr,               4,  S_RET(rdec),    S_UARG(fd),                    S_NARG(cstr, "pathname"),        S_NARG(ptr, "value"),             S_NARG(dec, "size")),
    SYSCALL(listxattr,               3,  S_RET(rdec),    S_NARG(cstr, "pathname"),      S_NARG(ptr, "list"),             S_NARG(dec, "size")),
    SYSCALL(llistxattr,              3,  S_RET(rdec),    S_NARG(cstr, "pathname"),      S_NARG(ptr, "list"),             S_NARG(dec, "size")),
    SYSCALL(flistxattr,              3,  S_RET(rdec),    S_UARG(fd),                    S_NARG(ptr, "list"),             S_NARG(dec, "size")),
    SYSCALL(removexattr,             2,  S_RET(rdec),    S_NARG(cstr, "pathname"),      S_NARG(cstr, "pathname")),
    SYSCALL(lremovexattr,            2,  S_RET(rdec),    S_NARG(cstr, "pathname"),      S_NARG(cstr, "pathname")),
    SYSCALL(fremovexattr,            2,  S_RET(rdec),    S_UARG(fd),                    S_NARG(cstr, "pathname")),
    SYSCALL(tkill,                   2,  S_RET(rdec),    S_NARG(dec, "pid"),            S_NARG(signum, "sig")),
    SYSCALL(time,                    1,  S_RET(rdec),    S_NARG(ptr, "tloc")),
    SYSCALL(futex,                   6,  S_RET(rdec),    S_NARG(ptr, "uaddr"),          S_NARG(arg, "op"),               S_NARG(arg, "val"),               S_NARG(ptr, "utime"),        S_NARG(ptr, "uaddr2"),       S_NARG(arg, "val3")),
    SYSCALL(sched_setaffinity,       3,  S_RET(rdec),    S_NARG(dec, "pid"),            S_NARG(arg, "length"),           S_NARG(ptr, "mask")),
    SYSCALL(sched_getaffinity,       3,  S_RET(rdec),    S_NARG(dec, "pid"),            S_NARG(arg, "length"),           S_NARG(ptr, "mask")),
    SYSCALL(set_thread_area,         1,  S_RET(rdec),    S_NARG(ptr, "u_info")),
    SYSCALL(io_setup,                2,  S_RET(rdec),    S_NARG(dec, "nr_reqs"),        S_NARG(ptr, "ctx")),
    SYSCALL(io_destroy,              1,  S_RET(rdec),    S_NARG(ptr, "ctx")),
    SYSCALL(io_getevents,            5,  S_RET(rdec),    S_NARG(ptr, "ctx_id"),         S_NARG(dec, "min_nr"),           S_NARG(dec, "nr"),                S_NARG(ptr, "events"),       S_NARG(ptr, "timeout")),
    SYSCALL(io_submit,               3,  S_RET(rdec),    S_NARG(ptr, "ctx_id"),         S_NARG(dec, "nr"),               S_NARG(ptr, "iocbpp")),
    SYSCALL(io_cancel,               3,  S_RET(rdec),    S_NARG(ptr, "ctx_id"),         S_NARG(ptr, "iocb"),             S_NARG(ptr, "result")),
    SYSCALL(get_thread_area,         1,  S_RET(rdec),    S_NARG(ptr, "u_info")),
    SYSCALL(lookup_dcookie,          3,  S_RET(rdec),    S_NARG(arg, "cookie64"),       S_NARG(ptr, "buf"),              S_NARG(dec, "length")),
    SYSCALL(epoll_create,            3,  S_RET(rdec),    S_NARG(arg, "size")),
    SYSCALL(epoll_ctl_old,           4,  S_RET(rdec),    S_NARG(arg, "arg0"),           S_NARG(arg, "arg1"),             S_NARG(arg, "arg2"),              S_NARG(arg, "arg3")),
    SYSCALL(epoll_wait_old,          4,  S_RET(rdec),    S_NARG(arg, "arg0"),           S_NARG(arg, "arg1"),             S_NARG(arg, "arg2"),              S_NARG(arg, "arg3")),
    SYSCALL(remap_file_pages,        5,  S_RET(rdec),    S_NARG(ptr, "addr"),           S_NARG(dec, "size"),             S_NARG(mmap_prot, "prot"),        S_NARG(dec, "pgoff"),        S_NARG(arg, "flags")),
    SYSCALL(getdents64,              3,  S_RET(rdec),    S_UARG(fd),                    S_NARG(ptr, "dirent"),           S_NARG(arg, "count")),
    SYSCALL(set_tid_address,         1,  S_RET(rdec),    S_NARG(ptr, "tidptr")),
    SYSCALL(restart_syscall,         0,  S_RET(rdec),    S_NOARGS()),
    SYSCALL(semtimedop,              4,  S_RET(rdec),    S_NARG(dec, "semid"),          S_NARG(ptr, "sops"),             S_NARG(arg, "nsops"),             S_NARG(ptr, "timeout")),
    SYSCALL(fadvise64,               4,  S_RET(rdec),    S_UARG(fd),                    S_UARG(offset),                  S_NARG(dec, "length"),            S_NARG(arg, "advice")),
    SYSCALL(timer_create,            3,  S_RET(rdec),    S_NARG(arg, "which_clock"),    S_NARG(ptr, "timer_event_spec"), S_NARG(ptr, "created_timer_id")),
    SYSCALL(timer_settime,           4,  S_RET(rdec),    S_NARG(arg, "timer_id"),       S_NARG(arg, "flags"),            S_NARG(ptr, "new_setting"),       S_NARG(ptr, "old_setting")),
    SYSCALL(timer_gettime,           2,  S_RET(rdec),    S_NARG(arg, "timer_id"),       S_NARG(ptr, "setting")),
    SYSCALL(timer_getoverrun,        1,  S_RET(rdec),    S_NARG(arg, "timer_id")),
    SYSCALL(timer_delete,            1,  S_RET(rdec),    S_NARG(arg, "timer_id")),
    SYSCALL(clock_settime,           2,  S_RET(rdec),    S_NARG(arg, "which_clock"),    S_NARG(ptr, "tp")),
    SYSCALL(clock_gettime,           2,  S_RET(rdec),    S_NARG(arg, "which_clock"),    S_NARG(ptr, "tp")),
    SYSCALL(clock_getres,            2,  S_RET(rdec),    S_NARG(arg, "which_clock"),    S_NARG(ptr, "tp")),
    SYSCALL(clock_nanosleep,         4,  S_RET(rdec),    S_NARG(arg, "which_clock"),    S_NARG(arg, "flags"),            S_NARG(ptr, "rqtp"),              S_NARG(ptr, "rmtp")),
    SYSCALL(exit_group,              1,  S_RET(rnone),   S_NARG(dec, "status")),
    SYSCALL(epoll_wait,              4,  S_RET(rdec),    S_NARG(dec, "epfd"),           S_NARG(ptr, "events"),           S_NARG(dec, "maxevents"),         S_NARG(dec32, "timeout")),
    SYSCALL(epoll_ctl,               4,  S_RET(rdec),    S_NARG(dec, "epfd"),           S_NARG(arg, "op"),               S_UARG(fd),                       S_NARG(ptr, "event")),
    SYSCALL(tgkill,                  3,  S_RET(rdec),    S_NARG(arg, "tgid"),           S_NARG(dec, "pid"),              S_NARG(signum, "sig")),
    SYSCALL(utimes,                  2,  S_RET(rdec),    S_NARG(cstr, "filename"),      S_NARG(ptr, "utimes")),
    SYSCALL(vserver,                 5,  S_RET(rdec),    S_NARG(arg, "arg0"),           S_NARG(arg, "arg1"),             S_NARG(arg, "arg2"),              S_NARG(arg, "arg3"),         S_NARG(arg, "arg4")),
    SYSCALL(mbind,                   6,  S_RET(rdec),    S_NARG(ptr, "addr"),           S_NARG(dec, "length"),           S_NARG(octal_mode, "mode"),       S_NARG(ptr, "nmask"),        S_NARG(arg, "maxnode"),      S_NARG(arg, "flags")),
    SYSCALL(set_mempolicy,           3,  S_RET(rdec),    S_NARG(octal_mode, "mode"),    S_NARG(ptr, "nmask"),            S_NARG(arg, "maxnode")),
    SYSCALL(get_mempolicy,           5,  S_RET(rdec),    S_NARG(ptr, "policy"),         S_NARG(ptr, "nmask"),            S_NARG(arg, "maxnode"),           S_NARG(ptr, "addr"),         S_NARG(arg, "flags")),
    SYSCALL(mq_open,                 4,  S_RET(rdec),    S_NARG(cstr, "pathname"),      S_NARG(open_flags, "oflag"),     S_NARG(octal_mode, "mode"),       S_NARG(ptr, "attr")),
    SYSCALL(mq_unlink,               1,  S_RET(rdec),    S_NARG(cstr, "pathname")),
    SYSCALL(mq_timedsend,            5,  S_RET(rdec),    S_NARG(arg, "mqdes"),          S_NARG(cstr, "msg_ptr"),         S_NARG(dec, "msg_len"),           S_NARG(arg, "msg_prio"),     S_NARG(ptr, "abs_timeout")),
    SYSCALL(mq_timedreceive,         5,  S_RET(rdec),    S_NARG(arg, "mqdes"),          S_NARG(ptr, "msg_ptr"),          S_NARG(dec, "msg_len"),           S_NARG(ptr, "msg_prio"),     S_NARG(ptr, "abs_timeout")),
    SYSCALL(mq_notify,               2,  S_RET(rdec),    S_NARG(arg, "mqdes"),          S_NARG(ptr, "notification")),
    SYSCALL(mq_getsetattr,           3,  S_RET(rdec),    S_NARG(arg, "mqdes"),          S_NARG(ptr, "mqstat"),           S_NARG(ptr, "omqstat")),
    SYSCALL(kexec_load,              4,  S_RET(rdec),    S_NARG(arg, "entry"),          S_NARG(arg, "nr_segments"),      S_NARG(ptr, "segments"),          S_NARG(arg, "flags")),
    SYSCALL(waitid,                  5,  S_RET(rdec),    S_NARG(arg, "which"),          S_NARG(dec, "pid"),              S_NARG(ptr, "infop"),             S_NARG(arg, "options"),      S_NARG(ptr, "ru")),
    SYSCALL(add_key,                 5,  S_RET(rdec),    S_NARG(cstr, "type"),          S_NARG(cstr, "description"),     S_NARG(ptr, "payload"),           S_NARG(dec, "plen"),         S_NARG(arg, "destringid")),
    SYSCALL(request_key,             4,  S_RET(rdec),    S_NARG(cstr, "type"),          S_NARG(cstr, "description"),     S_NARG(cstr, "callout_info"),     S_NARG(arg, "destringid")),
    SYSCALL(keyctl,                  5,  S_RET(rdec),    S_NARG(arg, "cmd"),            S_NARG(arg, "arg2"),             S_NARG(arg, "arg3"),              S_NARG(arg, "arg4"),         S_NARG(arg, "arg5")),
    SYSCALL(ioprio_set,              3,  S_RET(rdec),    S_NARG(arg, "which"),          S_NARG(arg, "who"),              S_NARG(dec, "ioprio")),
    SYSCALL(ioprio_get,              2,  S_RET(rdec),    S_NARG(arg, "which"),          S_NARG(arg, "who")),
    SYSCALL(inotify_init,            0,  S_RET(rdec),    S_NOARGS()),
    SYSCALL(inotify_add_watch,       3,  S_RET(rdec),    S_UARG(fd),                    S_NARG(cstr, "pathname"),        S_NARG(arg, "mask")),
    SYSCALL(inotify_rm_watch,        2,  S_RET(rdec),    S_UARG(fd),                    S_NARG(dec, "wd")),
    SYSCALL(migrate_pages,           4,  S_RET(rdec),    S_NARG(dec, "pid"),            S_NARG(arg, "maxnode"),          S_NARG(ptr, "from"),              S_NARG(ptr, "to")),
    SYSCALL(openat,                  3,  S_RET(rdec),    S_NARG(atfd, "dfd"),           S_NARG(cstr, "pathname"),        S_NARG(open_flags, "flags")),
    SYSCALL(mkdirat,                 3,  S_RET(rdec),    S_NARG(atfd, "dfd"),           S_NARG(cstr, "pathname"),        S_NARG(octal_mode, "mode")),
    SYSCALL(mknodat,                 4,  S_RET(rdec),    S_NARG(atfd, "dfd"),           S_NARG(cstr, "filename"),        S_NARG(octal_mode, "mode"),       S_NARG(arg, "dev")),
    SYSCALL(fchownat,                5,  S_RET(rdec),    S_NARG(atfd, "dfd"),           S_NARG(cstr, "pathname"),        S_NARG(dec, "user"),              S_NARG(dec, "group"),        S_NARG(arg, "flag")),
    SYSCALL(futimesat,               3,  S_RET(rdec),    S_NARG(atfd, "dfd"),           S_NARG(cstr, "pathname"),        S_NARG(ptr, "utimes")),
    SYSCALL(newfstatat,              4,  S_RET(rdec),    S_NARG(atfd, "dfd"),           S_NARG(cstr, "pathname"),        S_NARG(ptr, "statbuf"),           S_NARG(arg, "flag")),
    SYSCALL(unlinkat,                3,  S_RET(rdec),    S_NARG(atfd, "dfd"),           S_NARG(cstr, "pathname"),        S_NARG(arg, "flag")),
    SYSCALL(renameat,                4,  S_RET(rdec),    S_NARG(atfd, "olddfd"),        S_NARG(cstr, "oldname"),         S_NARG(atfd, "newdfd"),           S_NARG(cstr, "newname")),
    SYSCALL(linkat,                  5,  S_RET(rdec),    S_NARG(atfd, "olddfd"),        S_NARG(cstr, "oldpath"),         S_NARG(atfd, "newdfd"),           S_NARG(cstr, "newpath"),     S_NARG(arg, "flags")),
    SYSCALL(symlinkat,               3,  S_RET(rdec),    S_NARG(cstr, "oldname"),       S_NARG(atfd, "newdfd"),          S_NARG(cstr, "newname")),
    SYSCALL(readlinkat,              4,  S_RET(rdec),    S_NARG(atfd, "dfd"),           S_NARG(cstr, "pathname"),        S_NARG(ptr, "buf"),               S_NARG(arg, "bufsiz")),
    SYSCALL(fchmodat,                3,  S_RET(rdec),    S_NARG(atfd, "dfd"),           S_NARG(cstr, "filename"),        S_NARG(octal_mode, "mode")),
    SYSCALL(faccessat,               3,  S_RET(rdec),    S_NARG(atfd, "dfd"),           S_NARG(cstr, "pathname"),        S_NARG(octal_mode, "mode")),
    SYSCALL(pselect6,                6,  S_RET(rdec),    S_NARG(dec, "nfds"),           S_NARG(ptr, "readfds"),          S_NARG(ptr, "writefds"),          S_NARG(ptr, "exceptfds"),    S_NARG(ptr, "timeval"),      S_NARG(ptr, "sigmask")),
    SYSCALL(ppoll,                   5,  S_RET(rdec),    S_NARG(ptr, "fds"),            S_NARG(dec, "nfds"),             S_NARG(ptr, "tmo_p"),             S_NARG(ptr, "sigmask"),      S_NARG(dec, "sigsetsize")),
    SYSCALL(unshare,                 1,  S_RET(rdec),    S_NARG(arg, "unshare_flags")),
    SYSCALL(set_robust_list,         2,  S_RET(rdec),    S_NARG(ptr, "head"),           S_NARG(dec, "length")),
    SYSCALL(get_robust_list,         3,  S_RET(rdec),    S_NARG(dec, "pid"),            S_NARG(ptr, "head_ptr"),         S_NARG(ptr, "len_ptr")),
    SYSCALL(splice,                  6,  S_RET(rdec),    S_NARG(dec, "fd_in"),          S_NARG(ptr, "off_in"),           S_NARG(dec, "fd_out"),            S_NARG(ptr, "off_out"),      S_NARG(dec, "length"),       S_NARG(arg, "flags")),
    SYSCALL(tee,                     4,  S_RET(rdec),    S_NARG(dec, "fd_in"),          S_NARG(dec, "fd_out"),           S_NARG(dec, "length"),            S_NARG(arg, "flags")),
    SYSCALL(sync_file_range,         4,  S_RET(rdec),    S_UARG(fd),                    S_UARG(offset),                  S_NARG(offset, "nbytes"),          S_NARG(arg, "flags")),
    SYSCALL(vmsplice,                4,  S_RET(rdec),    S_UARG(fd),                    S_NARG(ptr, "iov"),              S_NARG(arg, "nr_segs"),           S_NARG(arg, "flags")),
    SYSCALL(move_pages,              6,  S_RET(rdec),    S_NARG(dec, "pid"),            S_NARG(arg, "nr_pages"),         S_NARG(ptr, "pages"),             S_NARG(ptr, "nodes"),        S_NARG(ptr, "status"),       S_NARG(arg, "flags")),
    SYSCALL(utimensat,               4,  S_RET(rdec),    S_NARG(atfd, "dfd"),           S_NARG(cstr, "pathname"),        S_NARG(ptr, "utimes"),            S_NARG(arg, "flags")),
    SYSCALL(epoll_pwait,             6,  S_RET(rdec),    S_NARG(fd, "epfd"),            S_NARG(ptr, "events"),           S_NARG(dec, "maxevents"),         S_NARG(dec, "timeout"),      S_NARG(ptr, "sigmask"),      S_NARG(dec, "sigsetsize")),
    SYSCALL(signalfd,                3,  S_RET(rdec),    S_NARG(dec, "ufd"),            S_NARG(ptr, "user_mask"),        S_NARG(dec, "sizemask")),
    SYSCALL(timerfd_create,          2,  S_RET(rdec),    S_NARG(dec, "clockid"),        S_NARG(arg, "flags")),
    SYSCALL(eventfd,                 1,  S_RET(rdec),    S_NARG(arg, "count")),
    SYSCALL(fallocate,               4,  S_RET(rdec),    S_UARG(fd),                    S_NARG(octal_mode, "mode"),      S_UARG(offset),                   S_NARG(offset, "length")),
    SYSCALL(timerfd_settime,         4,  S_RET(rdec),    S_NARG(fd, "ufd"),             S_NARG(arg, "flags"),            S_NARG(ptr, "utmr"),              S_NARG(ptr, "otmr")),
    SYSCALL(timerfd_gettime,         2,  S_RET(rdec),    S_NARG(fd, "ufd"),             S_NARG(ptr, "otmr")),
    SYSCALL(accept4,                 4,  S_RET(rdec),    S_NARG(fd, "sockfd"),          S_NARG(ptr, "addr"),             S_NARG(ptr, "addrlen"),           S_NARG(arg, "flags")),
    SYSCALL(signalfd4,               4,  S_RET(rdec),    S_NARG(fd, "ufd"),             S_NARG(ptr, "user_mask"),        S_NARG(dec, "sizemask"),          S_NARG(arg, "flags")),
    SYSCALL(eventfd2,                2,  S_RET(rdec),    S_NARG(arg, "count"),          S_NARG(arg, "flags")),
    SYSCALL(epoll_create1,           1,  S_RET(rdec),    S_NARG(arg, "flags")),
    SYSCALL(dup3,                    3,  S_RET(rdec),    S_NARG(fd, "oldfd"),           S_NARG(fd, "newfd"),             S_NARG(arg, "flags")),
    SYSCALL(pipe2,                   2,  S_RET(rdec),    S_NARG(ptr, "fildes"),         S_NARG(arg, "flags")),
    SYSCALL(inotify_init1,           1,  S_RET(rdec),    S_NARG(arg, "flags")),
    SYSCALL(preadv,                  5,  S_RET(rdec),    S_UARG(fd),                    S_NARG(ptr, "iov"),              S_NARG(dec, "iovcnt"),            S_NARG(arg, "pos_l"),        S_NARG(arg, "pos_h")),
    SYSCALL(pwritev,                 5,  S_RET(rdec),    S_UARG(fd),                    S_NARG(ptr, "iov"),              S_NARG(dec, "iovcnt"),            S_NARG(arg, "pos_l"),        S_NARG(arg, "pos_h")),
    SYSCALL(rt_tgsigqueueinfo,       4,  S_RET(rdec),    S_NARG(arg, "tgid"),           S_NARG(arg, "pid"),              S_NARG(signum, "sig"),            S_NARG(ptr, "uinfo")),
    SYSCALL(perf_event_open,         5,  S_RET(rdec),    S_NARG(ptr, "attr_uptr"),      S_NARG(dec, "pid"),              S_NARG(dec, "cpu"),               S_NARG(fd, "group_fd"),      S_NARG(arg, "flags")),
    SYSCALL(recvmmsg,                5,  S_RET(rdec),    S_UARG(fd),                    S_NARG(ptr, "msg"),              S_NARG(dec, "vlen"),              S_NARG(arg, "flags"),        S_NARG(ptr, "timeout")),
    SYSCALL(fanotify_init,           2,  S_RET(rdec),    S_NARG(arg, "flags"),          S_NARG(arg, "event_f_flags")),
    SYSCALL(fanotify_mark,           5,  S_RET(rdec),    S_NARG(fd, "fanotify_fd"),     S_NARG(arg, "flags"),            S_NARG(arg, "mask"),              S_UARG(fd),                  S_NARG(cstr, "pathname")),
    SYSCALL(prlimit64,               4,  S_RET(rdec),    S_NARG(dec, "pid"),            S_NARG(arg, "resource"),         S_NARG(ptr, "new_rlim"),          S_NARG(ptr, "old_rlim")),
    SYSCALL(name_to_handle_at,       5,  S_RET(rdec),    S_NARG(atfd, "dfd"),           S_NARG(cstr, "pathname"),        S_NARG(ptr, "handle"),            S_NARG(ptr, "mnt_id"),       S_NARG(arg, "flag")),
    SYSCALL(open_by_handle_at,       3,  S_RET(rdec),    S_NARG(fd, "mountdirfd"),      S_NARG(ptr, "handle"),           S_NARG(arg, "flags")),
    SYSCALL(clock_adjtime,           2,  S_RET(rdec),    S_NARG(arg, "which_clock"),    S_NARG(ptr, "tx")),
    SYSCALL(syncfs,                  2,  S_RET(rdec),    S_UARG(fd)),
    SYSCALL(sendmmsg,                4,  S_RET(rdec),    S_UARG(fd),                    S_NARG(ptr, "msg"),              S_NARG(dec, "vlen"),              S_NARG(arg, "flags")),
    SYSCALL(setns,                   2,  S_RET(rdec),    S_UARG(fd),                    S_NARG(arg, "nstype")),
    SYSCALL(getcpu,                  3,  S_RET(rdec),    S_NARG(ptr, "cpu"),            S_NARG(ptr, "node"),             S_NARG(ptr, "cache")),
    SYSCALL(process_vm_readv,        6,  S_RET(rdec),    S_NARG(dec, "pid"),            S_NARG(ptr, "local_iov"),        S_NARG(dec, "liovcnt"),           S_NARG(ptr, "remote_iov"),   S_NARG(dec, "riovcnt"),      S_NARG(arg, "flags")),
    SYSCALL(process_vm_writev,       6,  S_RET(rdec),    S_NARG(dec, "pid"),            S_NARG(ptr, "local_iov"),        S_NARG(dec, "liovcnt"),           S_NARG(ptr, "remote_iov"),   S_NARG(dec, "riovcnt"),      S_NARG(arg, "flags")),
    SYSCALL(kcmp,                    5,  S_RET(rdec),    S_NARG(arg, "pid1"),           S_NARG(arg, "pid2"),             S_NARG(arg, "type"),              S_NARG(arg, "idx1"),         S_NARG(arg, "idx2")),
    SYSCALL(finit_module,            3,  S_RET(rdec),    S_UARG(fd),                    S_NARG(cstr, "param_values"),    S_NARG(arg, "flags")),
    SYSCALL(sched_setattr,           3,  S_RET(rdec),    S_NARG(dec, "pid"),            S_NARG(ptr, "attr"),             S_NARG(arg, "flags")),
    SYSCALL(sched_getattr,           4,  S_RET(rdec),    S_NARG(dec, "pid"),            S_NARG(ptr, "attr"),             S_NARG(dec, "size"),              S_NARG(arg, "flags")),
    SYSCALL(renameat2,               5,  S_RET(rdec),    S_NARG(atfd, "olddfd"),        S_NARG(cstr, "oldpath"),         S_NARG(atfd, "newdfd"),           S_NARG(cstr, "newpath"),     S_NARG(arg, "flags")),
    SYSCALL(seccomp,                 3,  S_RET(rdec),    S_NARG(arg, "op"),             S_NARG(arg, "flags"),            S_NARG(ptr, "uargs")),
    SYSCALL(getrandom,               3,  S_RET(rdec),    S_NARG(ptr, "buf"),            S_NARG(arg, "count"),            S_NARG(arg, "flags")),
    SYSCALL(memfd_create,            2,  S_RET(rdec),    S_NARG(cstr, "pathname"),      S_NARG(arg, "flags")),
    SYSCALL(kexec_file_load,         5,  S_RET(rdec),    S_NARG(fd, "kernel_fd"),       S_NARG(fd, "initrd_fd"),         S_NARG(arg, "cmdline_len"),       S_NARG(cstr, "cmdline"),     S_NARG(arg, "flags")),

#ifdef SYS_bpf
    SYSCALL(bpf,                     2,  S_RET(rdec),    S_NARG(arg, "cmd"),            S_NARG(ptr, "attr"),             S_NARG(arg, "size")),
#endif // SYS_bpf

#ifdef SYS_execveat 
    SYSCALL(execveat,                5,  S_RET(rdec),    S_NARG(atfd, "dfd"),           S_NARG(cstr, "pathname"),        S_NARG(ptr, "argv"),              S_NARG(ptr, "envp"),         S_NARG(arg, "flags")),
#endif // SYS_execveat

    SYSCALL(userfaultfd,             2,  S_RET(rdec),    S_NARG(arg, "flags")),

#ifdef SYS_membarrier
    SYSCALL(membarrier,              2,  S_RET(rdec),    S_NARG(arg, "cmd"),            S_NARG(arg, "flags")),
#endif // SYS_membarrier

#ifdef SYS_mlock2
    SYSCALL(mlock2,                  3,  S_RET(rdec),    S_NARG(ptr, "addr"),           S_NARG(dec, "length"),           S_NARG(arg, "flags")),
#endif // SYS_mlock2

    SYSCALL(copy_file_range,         6,  S_RET(rdec),    S_NARG(fd, "fd_in"),           S_NARG(ptr, "off_in"),           S_NARG(fd, "fd_out"),             S_NARG(ptr, "off_out"),      S_NARG(dec, "length"),       S_NARG(arg, "flags")),

#ifdef SYS_preadv2
    SYSCALL(preadv2,                 6,  S_RET(rdec),    S_UARG(fd),                    S_NARG(ptr, "iov"),              S_NARG(arg, "vlen"),              S_NARG(arg, "pos_l"),        S_NARG(arg, "pos_h"),          S_NARG(arg, "flags")),
#endif // SYS_preadv2

#ifdef SYS_pwritev2
    SYSCALL(pwritev2,                6,  S_RET(rdec),    S_UARG(fd),                    S_NARG(ptr, "iov"),              S_NARG(arg, "vlen"),              S_NARG(arg, "pos_l"),        S_NARG(arg, "pos_h"),          S_NARG(arg, "flags")),
#endif // SYS_pwritev2

    SYSCALL(pkey_mprotect,           4,  S_RET(rdec),    S_NARG(ptr, "addr"),           S_NARG(dec, "length"),           S_NARG(mmap_prot, "prot"),        S_NARG(dec, "pkey")),
    SYSCALL(pkey_alloc,              2,  S_RET(rdec),    S_NARG(arg, "flags"),          S_NARG(arg, "init_val")),
    SYSCALL(pkey_free,               1,  S_RET(rdec),    S_NARG(dec, "pkey")),

#ifdef SYS_statx
    SYSCALL(statx,                   5,  S_RET(rdec),    S_NARG(atfd, "dfd"),           S_NARG(cstr, "pathname"),        S_NARG(arg, "flags"),             S_NARG(arg, "mask"),         S_NARG(ptr, "buffer")),
#endif // SYS_statx

#ifdef SYS_io_pgetevents
    SYSCALL(io_pgetevents,           6,  S_RET(rdec),    S_NARG(ptr, "ctx_id"),         S_NARG(dec, "min_nr"),           S_NARG(dec, "nr"),                S_NARG(ptr, "events"),       S_NARG(ptr, "timeout"),      S_NARG(ptr, "sig")),
#endif // SYS_io_pgetevents

#ifdef SYS_rseq
    SYSCALL(rseq,                    4,  S_RET(rdec),    S_NARG(ptr, "rseq"),           S_NARG(dec, "rseq_len"),         S_NARG(arg, "flags"),             S_NARG(signum, "sig"))
#endif // SYS_rseq
};


static const struct syscall_info unknown_syscall = {
    .s_name = "unknown_syscall",
    .s_nargs = MAX_SYSCALL_ARGS,
    .s_return_type = S_RET(rdec),
    .s_args = {
        S_NARG(arg, "arg0"),
        S_NARG(arg, "arg1"),
        S_NARG(arg, "arg2"),
        S_NARG(arg, "arg3"),
        S_NARG(arg, "arg4"),
        S_NARG(arg, "arg5"),
    }
};

static const struct syscall_info open_with_o_creat = {
    .s_name = "open",
    .s_nargs = 3,
    .s_return_type = S_RET(rdec),
	.s_args = {
	    S_NARG(cstr, "pathname"),
	    S_NARG(open_flags, "flags"),
	    S_NARG(octal_mode, "mode")
	}
};

static const struct syscall_info openat_with_o_creat = {
    .s_name = "openat", 
    .s_nargs = 4,
    .s_return_type = S_RET(rdec),
	.s_args = {
	    S_NARG(atfd, "dfd"), 
	    S_NARG(cstr, "pathname"),
	    S_NARG(open_flags, "flags"),
	    S_NARG(octal_mode, "mode")
	}
};

static bool
requires_mode_arg(int flags) {

    if((flags & O_CREAT) == O_CREAT) {
        return true;
    }

#ifdef O_TMPFILE
    if ((flags & O_TMPFILE) == O_TMPFILE) {
        return true;
    }
#endif

    return false;
}

#include <signal.h>

/** 
 * get_syscall_info - Return a syscall descriptor 
 *
 * This function returns a pointer to a syscall_info structure that 
 * appropriately describes the system call identified by 'syscall_number'.
 */
const struct syscall_info *
get_syscall_info(const long syscall_number, 
                 const long* argv) {

    if(syscall_number < 0 || 
       syscall_number >= (long) ARRAY_SIZE(syscall_table)) {
        return &unknown_syscall;
    }

    if(syscall_table[syscall_number].s_name == NULL) {
        return &unknown_syscall;
    }

    if(argv == NULL) {
        return &syscall_table[syscall_number];
    }

    if(syscall_number == SYS_open && requires_mode_arg(argv[1])) {
        return &open_with_o_creat;
    }

    if(syscall_number == SYS_openat && requires_mode_arg(argv[2])) {
        return &openat_with_o_creat;
    }

    return &syscall_table[syscall_number];
}

#define RETURN_TYPE(scinfo) \
    (scinfo)->s_return_type.r_type

bool
syscall_never_returns(long syscall_number) {
    return RETURN_TYPE(get_syscall_info(syscall_number, NULL)) == rnone;
}



#undef SYSCALL
#undef S_NOARGS
#undef S_UARG
#undef S_NARG
#undef S_RET
#undef ARRAY_SIZE
