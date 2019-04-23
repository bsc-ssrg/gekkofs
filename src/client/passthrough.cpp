/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  SPDX-License-Identifier: MIT
*/

#include "client/passthrough.hpp"

#include <stdlib.h>
#include <pthread.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/syscall.h>


/* We cannot use any fprintf here because it will call the write function internally
 * that we are also intercepting, and this will lead to a deadlock.
 *
 * Instead the write syscall needs to be called directly.
 */
#define WRITE_STDERR(str) \
    do { \
        syscall(SYS_write, 2, str, sizeof(str)); \
    } while (0)


#define LIBC_FUNC_LOAD(FNAME) \
    do { \
        LIBC_FUNC_NAME(FNAME) = dlsym(glibc, #FNAME); \
        if (LIBC_FUNC_NAME(FNAME) ==  nullptr) { \
            WRITE_STDERR("GekkoFS ERROR: failed to load glibc func symbol: '" #FNAME "'\n"); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)


static pthread_once_t init_lib_thread = PTHREAD_ONCE_INIT;


void* LIBC_FUNC_NAME(open);
void* LIBC_FUNC_NAME(openat);

void* LIBC_FUNC_NAME(fopen);
void* LIBC_FUNC_NAME(fopen64);
void* LIBC_FUNC_NAME(fread);
void* LIBC_FUNC_NAME(fwrite);
void* LIBC_FUNC_NAME(fclose);
void* LIBC_FUNC_NAME(clearerr);
void* LIBC_FUNC_NAME(feof);
void* LIBC_FUNC_NAME(ferror);
void* LIBC_FUNC_NAME(fileno);
void* LIBC_FUNC_NAME(fflush);
void* LIBC_FUNC_NAME(__fpurge);

void* LIBC_FUNC_NAME(setbuf);
void* LIBC_FUNC_NAME(setbuffer);
void* LIBC_FUNC_NAME(setlinebuf);
void* LIBC_FUNC_NAME(setvbuf);

void* LIBC_FUNC_NAME(putc);
void* LIBC_FUNC_NAME(fputc);
void* LIBC_FUNC_NAME(fputs);
void* LIBC_FUNC_NAME(getc);
void* LIBC_FUNC_NAME(fgetc);
void* LIBC_FUNC_NAME(fgets);
void* LIBC_FUNC_NAME(ungetc);

void* LIBC_FUNC_NAME(fseek);

void* LIBC_FUNC_NAME(mkdir);
void* LIBC_FUNC_NAME(mkdirat);
void* LIBC_FUNC_NAME(unlink);
void* LIBC_FUNC_NAME(unlinkat);
void* LIBC_FUNC_NAME(rmdir);

void* LIBC_FUNC_NAME(close);

void* LIBC_FUNC_NAME(access);
void* LIBC_FUNC_NAME(faccessat);

void* LIBC_FUNC_NAME(__xstat);
void* LIBC_FUNC_NAME(__xstat64);
void* LIBC_FUNC_NAME(__fxstat);
void* LIBC_FUNC_NAME(__fxstat64);
void* LIBC_FUNC_NAME(__fxstatat);
void* LIBC_FUNC_NAME(__fxstatat64);
void* LIBC_FUNC_NAME(__lxstat);
void* LIBC_FUNC_NAME(__lxstat64);

void* LIBC_FUNC_NAME(statfs);
void* LIBC_FUNC_NAME(fstatfs);
void* LIBC_FUNC_NAME(statvfs);
void* LIBC_FUNC_NAME(fstatvfs);

void* LIBC_FUNC_NAME(write);
void* LIBC_FUNC_NAME(pwrite);
void* LIBC_FUNC_NAME(pwrite64);
void* LIBC_FUNC_NAME(writev);

void* LIBC_FUNC_NAME(read);
void* LIBC_FUNC_NAME(pread);
void* LIBC_FUNC_NAME(pread64);
void* LIBC_FUNC_NAME(readv);

void* LIBC_FUNC_NAME(lseek);
void* LIBC_FUNC_NAME(lseek64);

void* LIBC_FUNC_NAME(fsync);
void* LIBC_FUNC_NAME(fdatasync);

void* LIBC_FUNC_NAME(truncate);
void* LIBC_FUNC_NAME(ftruncate);

void* LIBC_FUNC_NAME(fcntl);

void* LIBC_FUNC_NAME(dup);
void* LIBC_FUNC_NAME(dup2);
void* LIBC_FUNC_NAME(dup3);

void* LIBC_FUNC_NAME(dirfd);
void* LIBC_FUNC_NAME(opendir);
void* LIBC_FUNC_NAME(fdopendir);
void* LIBC_FUNC_NAME(readdir);
void* LIBC_FUNC_NAME(closedir);

void* LIBC_FUNC_NAME(chmod);
void* LIBC_FUNC_NAME(fchmod);
void* LIBC_FUNC_NAME(fchmodat);

void* LIBC_FUNC_NAME(chdir);
void* LIBC_FUNC_NAME(fchdir);

void* LIBC_FUNC_NAME(getcwd);
void* LIBC_FUNC_NAME(get_current_dir_name);

void* LIBC_FUNC_NAME(link);
void* LIBC_FUNC_NAME(linkat);
void* LIBC_FUNC_NAME(symlinkat);

void* LIBC_FUNC_NAME(readlinkat);
void* LIBC_FUNC_NAME(realpath);


void init_passthrough_() {
    auto glibc = dlopen("libc.so.6", RTLD_LAZY);
    if(glibc == nullptr) {
        WRITE_STDERR("GekkoFS ERROR: failed to load glibc\n"); \
        exit(EXIT_FAILURE);
    }

    LIBC_FUNC_LOAD(open);
    LIBC_FUNC_LOAD(openat);

    LIBC_FUNC_LOAD(fopen);
    LIBC_FUNC_LOAD(fopen64);
    LIBC_FUNC_LOAD(fread);
    LIBC_FUNC_LOAD(fwrite);
    LIBC_FUNC_LOAD(fclose);
    LIBC_FUNC_LOAD(clearerr);
    LIBC_FUNC_LOAD(feof);
    LIBC_FUNC_LOAD(ferror);
    LIBC_FUNC_LOAD(fileno);
    LIBC_FUNC_LOAD(fflush);
    LIBC_FUNC_LOAD(__fpurge);

    LIBC_FUNC_LOAD(setbuf);
    LIBC_FUNC_LOAD(setbuffer);
    LIBC_FUNC_LOAD(setlinebuf);
    LIBC_FUNC_LOAD(setvbuf);

    LIBC_FUNC_LOAD(putc);
    LIBC_FUNC_LOAD(fputc);
    LIBC_FUNC_LOAD(fputs);
    LIBC_FUNC_LOAD(getc);
    LIBC_FUNC_LOAD(fgetc);
    LIBC_FUNC_LOAD(fgets);
    LIBC_FUNC_LOAD(ungetc);

    LIBC_FUNC_LOAD(fseek);

    LIBC_FUNC_LOAD(mkdir);
    LIBC_FUNC_LOAD(mkdirat);

    LIBC_FUNC_LOAD(unlink);
    LIBC_FUNC_LOAD(unlinkat);
    LIBC_FUNC_LOAD(rmdir);

    LIBC_FUNC_LOAD(close);

    LIBC_FUNC_LOAD(access);
    LIBC_FUNC_LOAD(faccessat);

    LIBC_FUNC_LOAD(__xstat);
    LIBC_FUNC_LOAD(__xstat64);
    LIBC_FUNC_LOAD(__fxstat);
    LIBC_FUNC_LOAD(__fxstat64);
    LIBC_FUNC_LOAD(__fxstatat);
    LIBC_FUNC_LOAD(__fxstatat64);
    LIBC_FUNC_LOAD(__lxstat);
    LIBC_FUNC_LOAD(__lxstat64);

    LIBC_FUNC_LOAD(statfs);
    LIBC_FUNC_LOAD(fstatfs);
    LIBC_FUNC_LOAD(statvfs);
    LIBC_FUNC_LOAD(fstatvfs);

    LIBC_FUNC_LOAD(write);
    LIBC_FUNC_LOAD(pwrite);
    LIBC_FUNC_LOAD(pwrite64);
    LIBC_FUNC_LOAD(writev);

    LIBC_FUNC_LOAD(read);
    LIBC_FUNC_LOAD(pread);
    LIBC_FUNC_LOAD(pread64);
    LIBC_FUNC_LOAD(readv);

    LIBC_FUNC_LOAD(lseek);
    LIBC_FUNC_LOAD(lseek64);
    LIBC_FUNC_LOAD(fsync);
    LIBC_FUNC_LOAD(fdatasync);

    LIBC_FUNC_LOAD(truncate);
    LIBC_FUNC_LOAD(ftruncate);

    LIBC_FUNC_LOAD(fcntl);

    LIBC_FUNC_LOAD(dup);
    LIBC_FUNC_LOAD(dup2);
    LIBC_FUNC_LOAD(dup3);

    LIBC_FUNC_LOAD(dirfd);
    LIBC_FUNC_LOAD(opendir);
    LIBC_FUNC_LOAD(fdopendir);
    LIBC_FUNC_LOAD(readdir);
    LIBC_FUNC_LOAD(closedir);

    LIBC_FUNC_LOAD(chmod);
    LIBC_FUNC_LOAD(fchmod);
    LIBC_FUNC_LOAD(fchmodat);

    LIBC_FUNC_LOAD(chdir);
    LIBC_FUNC_LOAD(fchdir);

    LIBC_FUNC_LOAD(getcwd);
    LIBC_FUNC_LOAD(get_current_dir_name);

    LIBC_FUNC_LOAD(link);
    LIBC_FUNC_LOAD(linkat);
    LIBC_FUNC_LOAD(symlinkat);

    LIBC_FUNC_LOAD(readlinkat);

    LIBC_FUNC_LOAD(realpath);
}

void init_passthrough_if_needed() {
    pthread_once(&init_lib_thread, init_passthrough_);
}