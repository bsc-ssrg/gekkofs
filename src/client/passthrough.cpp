/**
 * All intercepted functions are mapped to a different function pointer prefixing <libc_>
 */
#include <client/passthrough.hpp>

#include <iostream>
#include <pthread.h>
#include <dlfcn.h>

static pthread_once_t init_lib_thread = PTHREAD_ONCE_INIT;

// function pointer for preloading
void* libc;

void* libc_open;
void* libc_openat;

void* libc_fopen;
void* libc_fopen64;
void* libc_fread;
void* libc_fwrite;
void* libc_fclose;
void* libc_clearerr;
void* libc_feof;
void* libc_ferror;
void* libc_fileno;
void* libc_fflush;
void* libc_fpurge;
void* libc___fpurge;

void* libc_setbuf;
void* libc_setbuffer;
void* libc_setlinebuf;
void* libc_setvbuf;

void* libc_putc;
void* libc_fputc;
void* libc_fputs;
void* libc_getc;
void* libc_fgetc;
void* libc_fgets;
void* libc_ungetc;

void* libc_fseek;

void* libc_mkdir;
void* libc_mkdirat;
void* libc_unlink;
void* libc_unlinkat;
void* libc_rmdir;

void* libc_close;

void* libc_access;
void* libc_faccessat;

void* libc_stat;
void* libc_fstat;
void* libc_lstat;
void* libc___xstat;
void* libc___xstat64;
void* libc___fxstat;
void* libc___fxstat64;
void* libc___fxstatat;
void* libc___fxstatat64;
void* libc___lxstat;
void* libc___lxstat64;

void* libc_statfs;
void* libc_fstatfs;
void* libc_statvfs;
void* libc_fstatvfs;

void* libc_write;
void* libc_pwrite;
void* libc_pwrite64;
void* libc_writev;

void* libc_read;
void* libc_pread;
void* libc_pread64;
void* libc_readv;

void* libc_lseek;
void* libc_lseek64;

void* libc_fsync;
void* libc_fdatasync;

void* libc_truncate;
void* libc_ftruncate;

void* libc_fcntl;

void* libc_dup;
void* libc_dup2;
void* libc_dup3;

void* libc_dirfd;
void* libc_opendir;
void* libc_fdopendir;
void* libc_readdir;
void* libc_closedir;

void* libc_chmod;
void* libc_fchmod;
void* libc_fchmodat;

void* libc_chdir;
void* libc_fchdir;

void* libc_getcwd;
void* libc_get_current_dir_name;

void* libc_link;
void* libc_linkat;
void* libc_symlinkat;

void* libc_readlinkat;

void* libc_realpath;

void* libc_rename;
void* libc___renameat;
void* libc_renameat2;


void init_passthrough_() {
    libc = dlopen("libc.so.6", RTLD_LAZY);
    if(libc == nullptr){
        std::cerr << "ERROR: failed opening glibc shared object: " << dlerror() << std::endl;
        exit(EXIT_FAILURE);
    }

    libc_open = dlsym(libc, "open");
    libc_openat = dlsym(libc, "openat");

    libc_fopen = dlsym(libc, "fopen");
    libc_fopen64 = dlsym(libc, "fopen64");
    libc_fread = dlsym(libc, "fread");
    libc_fwrite = dlsym(libc, "fwrite");
    libc_fclose = dlsym(libc, "fclose");
    libc_clearerr = dlsym(libc, "clearerr");
    libc_feof = dlsym(libc, "feof");
    libc_ferror = dlsym(libc, "ferror");
    libc_fileno = dlsym(libc, "fileno");
    libc_fflush = dlsym(libc, "fflush");
    libc_fpurge = dlsym(libc, "fpurge");
    libc___fpurge = dlsym(libc, "__fpurge");

    libc_setbuf = dlsym(libc, "setbuf");
    libc_setbuffer = dlsym(libc, "setbuffer");
    libc_setlinebuf = dlsym(libc, "setlinebuf");
    libc_setvbuf = dlsym(libc, "setvbuf");

    libc_putc = dlsym(libc, "putc");
    libc_fputc = dlsym(libc, "fputc");
    libc_fputs = dlsym(libc, "fputs");
    libc_getc = dlsym(libc, "getc");
    libc_fgetc = dlsym(libc, "fgetc");
    libc_fgets = dlsym(libc, "fgets");
    libc_ungetc = dlsym(libc, "ungetc");

    libc_fseek = dlsym(libc, "fseek");

    libc_mkdir = dlsym(libc, "mkdir");
    libc_mkdirat = dlsym(libc, "mkdirat");

    libc_unlink = dlsym(libc, "unlink");
    libc_unlinkat = dlsym(libc, "unlinkat");
    libc_rmdir = dlsym(libc, "rmdir");

    libc_close = dlsym(libc, "close");

    libc_access = dlsym(libc, "access");
    libc_faccessat = dlsym(libc, "faccessat");

    libc_stat = dlsym(libc, "stat");
    libc_fstat = dlsym(libc, "fstat");
    libc_lstat = dlsym(libc, "lstat");
    libc___xstat = dlsym(libc, "__xstat");
    libc___xstat64 = dlsym(libc, "__xstat64");
    libc___fxstat = dlsym(libc, "__fxstat");
    libc___fxstat64 = dlsym(libc, "__fxstat64");
    libc___fxstatat = dlsym(libc, "__fxstatat");
    libc___fxstatat64 = dlsym(libc, "__fxstatat64");
    libc___lxstat = dlsym(libc, "__lxstat");
    libc___lxstat64 = dlsym(libc, "__lxstat64");

    libc_statfs = dlsym(libc, "statfs");
    libc_fstatfs = dlsym(libc, "fstatfs");
    libc_statvfs = dlsym(libc, "statvfs");
    libc_fstatvfs = dlsym(libc, "fstatvfs");

    libc_write = dlsym(libc, "write");
    libc_pwrite = dlsym(libc, "pwrite");
    libc_pwrite64 = dlsym(libc, "pwrite64");
    libc_writev = dlsym(libc, "writev");

    libc_read = dlsym(libc, "read");
    libc_pread = dlsym(libc, "pread");
    libc_pread64 = dlsym(libc, "pread64");
    libc_readv = dlsym(libc, "readv");

    libc_lseek = dlsym(libc, "lseek");
    libc_lseek64 = dlsym(libc, "lseek64");
    libc_fsync = dlsym(libc, "fsync");
    libc_fdatasync = dlsym(libc, "fdatasync");

    libc_truncate = dlsym(libc, "truncate");
    libc_ftruncate = dlsym(libc, "ftruncate");

    libc_fcntl = dlsym(libc, "fcntl");

    libc_dup = dlsym(libc, "dup");
    libc_dup2 = dlsym(libc, "dup2");
    libc_dup3 = dlsym(libc, "dup3");

    libc_dirfd = dlsym(libc, "dirfd");
    libc_opendir = dlsym(libc, "opendir");
    libc_fdopendir = dlsym(libc, "fdopendir");
    libc_readdir = dlsym(libc, "readdir");
    libc_closedir = dlsym(libc, "closedir");

    libc_chmod = dlsym(libc, "chmod");
    libc_fchmod = dlsym(libc, "fchmod");
    libc_fchmodat = dlsym(libc, "fchmodat");

    libc_chdir = dlsym(libc, "chdir");
    libc_fchdir = dlsym(libc, "fchdir");

    libc_getcwd = dlsym(libc, "getcwd");
    libc_get_current_dir_name = dlsym(libc, "get_current_dir_name");

    libc_link = dlsym(libc, "link");
    libc_linkat = dlsym(libc, "linkat");
    libc_symlinkat = dlsym(libc, "symlinkat");

    libc_readlinkat = dlsym(libc, "readlinkat");

    libc_realpath = dlsym(libc, "realpath");

    libc_rename = dlsym(libc, "rename");
    libc___renameat = dlsym(libc, "__renameat");
    libc_renameat2 = dlsym(libc, "renameat2");
}

void init_passthrough_if_needed() {
    pthread_once(&init_lib_thread, init_passthrough_);
}