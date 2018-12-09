//
// Created by evie on 11/15/17.
//

#ifndef IFS_PASSTHROUGH_HPP
#define IFS_PASSTHROUGH_HPP


#define LIBC_FUNC(FNAME, ...) \
    ((reinterpret_cast<decltype(&FNAME)>(libc_##FNAME))(__VA_ARGS__))

// function pointer for preloading
extern void* libc;

extern void* libc_open;
extern void* libc_openat;

extern void* libc_fopen;
extern void* libc_fopen64;
extern void* libc_fread;
extern void* libc_fwrite;
extern void* libc_fclose;
extern void* libc_clearerr;
extern void* libc_feof;
extern void* libc_ferror;
extern void* libc_fileno;
extern void* libc_fflush;
extern void* libc_fpurge;
extern void* libc___fpurge;

extern void* libc_setbuf;
extern void* libc_setbuffer;
extern void* libc_setlinebuf;
extern void* libc_setvbuf;

extern void* libc_putc;
extern void* libc_fputc;
extern void* libc_fputs;
extern void* libc_getc;
extern void* libc_fgetc;
extern void* libc_fgets;
extern void* libc_ungetc;

extern void* libc_fseek;

extern void* libc_mkdir;
extern void* libc_mkdirat;
extern void* libc_unlink;
extern void* libc_unlinkat;
extern void* libc_rmdir;

extern void* libc_close;

extern void* libc_access;
extern void* libc_faccessat;

extern void* libc_stat;
extern void* libc_fstat;
extern void* libc_lstat;
extern void* libc___xstat;
extern void* libc___xstat64;
extern void* libc___fxstat;
extern void* libc___fxstat64;
extern void* libc___fxstatat;
extern void* libc___fxstatat64;
extern void* libc___lxstat;
extern void* libc___lxstat64;

extern void* libc_statfs;
extern void* libc_fstatfs;

extern void* libc_write;
extern void* libc_pwrite;
extern void* libc_pwrite64;
extern void* libc_writev;

extern void* libc_read;
extern void* libc_pread;
extern void* libc_pread64;
extern void* libc_readv;

extern void* libc_lseek;
extern void* libc_lseek64;
extern void* libc_fsync;
extern void* libc_fdatasync;

extern void* libc_truncate;
extern void* libc_ftruncate;

extern void* libc_fcntl;

extern void* libc_dup;
extern void* libc_dup2;
extern void* libc_dup3;

extern void* libc_dirfd;
extern void* libc_opendir;
extern void* libc_fdopendir;
extern void* libc_readdir;
extern void* libc_closedir;

extern void* libc_chmod;
extern void* libc_fchmod;
extern void* libc_fchmodat;

extern void* libc_chdir;
extern void* libc_fchdir;

extern void* libc_getcwd;
extern void* libc_get_current_dir_name;

extern void* libc_link;
extern void* libc_linkat;
extern void* libc_symlink;
extern void* libc_symlinkat;

extern void* libc_realpath;

extern void* libc_rename;
extern void* libc___renameat;
extern void* libc_renameat2;

void init_passthrough_if_needed();

#endif //IFS_PASSTHROUGH_HPP
