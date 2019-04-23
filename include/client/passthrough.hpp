/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  SPDX-License-Identifier: MIT
*/

#ifndef IFS_PASSTHROUGH_HPP
#define IFS_PASSTHROUGH_HPP


#define LIBC_FUNC_NAME(FNAME) libc_##FNAME

#define LIBC_FUNC(FNAME, ...) \
    ((reinterpret_cast<decltype(&FNAME)>(libc_##FNAME))(__VA_ARGS__))


extern void* LIBC_FUNC_NAME(open);
extern void* LIBC_FUNC_NAME(openat);

extern void* LIBC_FUNC_NAME(fopen);
extern void* LIBC_FUNC_NAME(fopen64);
extern void* LIBC_FUNC_NAME(fread);
extern void* LIBC_FUNC_NAME(fwrite);
extern void* LIBC_FUNC_NAME(fclose);
extern void* LIBC_FUNC_NAME(clearerr);
extern void* LIBC_FUNC_NAME(feof);
extern void* LIBC_FUNC_NAME(ferror);
extern void* LIBC_FUNC_NAME(fileno);
extern void* LIBC_FUNC_NAME(fflush);
extern void* LIBC_FUNC_NAME(__fpurge);

extern void* LIBC_FUNC_NAME(setbuf);
extern void* LIBC_FUNC_NAME(setbuffer);
extern void* LIBC_FUNC_NAME(setlinebuf);
extern void* LIBC_FUNC_NAME(setvbuf);

extern void* LIBC_FUNC_NAME(putc);
extern void* LIBC_FUNC_NAME(fputc);
extern void* LIBC_FUNC_NAME(fputs);
extern void* LIBC_FUNC_NAME(getc);
extern void* LIBC_FUNC_NAME(fgetc);
extern void* LIBC_FUNC_NAME(fgets);
extern void* LIBC_FUNC_NAME(ungetc);

extern void* LIBC_FUNC_NAME(fseek);

extern void* LIBC_FUNC_NAME(mkdir);
extern void* LIBC_FUNC_NAME(mkdirat);
extern void* LIBC_FUNC_NAME(unlink);
extern void* LIBC_FUNC_NAME(unlinkat);
extern void* LIBC_FUNC_NAME(rmdir);

extern void* LIBC_FUNC_NAME(close);

extern void* LIBC_FUNC_NAME(access);
extern void* LIBC_FUNC_NAME(faccessat);

extern void* LIBC_FUNC_NAME(__xstat);
extern void* LIBC_FUNC_NAME(__xstat64);
extern void* LIBC_FUNC_NAME(__fxstat);
extern void* LIBC_FUNC_NAME(__fxstat64);
extern void* LIBC_FUNC_NAME(__fxstatat);
extern void* LIBC_FUNC_NAME(__fxstatat64);
extern void* LIBC_FUNC_NAME(__lxstat);
extern void* LIBC_FUNC_NAME(__lxstat64);

extern void* LIBC_FUNC_NAME(statfs);
extern void* LIBC_FUNC_NAME(fstatfs);
extern void* LIBC_FUNC_NAME(statvfs);
extern void* LIBC_FUNC_NAME(fstatvfs);

extern void* LIBC_FUNC_NAME(write);
extern void* LIBC_FUNC_NAME(pwrite);
extern void* LIBC_FUNC_NAME(pwrite64);
extern void* LIBC_FUNC_NAME(writev);

extern void* LIBC_FUNC_NAME(read);
extern void* LIBC_FUNC_NAME(pread);
extern void* LIBC_FUNC_NAME(pread64);
extern void* LIBC_FUNC_NAME(readv);

extern void* LIBC_FUNC_NAME(lseek);
extern void* LIBC_FUNC_NAME(lseek64);
extern void* LIBC_FUNC_NAME(fsync);
extern void* LIBC_FUNC_NAME(fdatasync);

extern void* LIBC_FUNC_NAME(truncate);
extern void* LIBC_FUNC_NAME(ftruncate);

extern void* LIBC_FUNC_NAME(fcntl);

extern void* LIBC_FUNC_NAME(dup);
extern void* LIBC_FUNC_NAME(dup2);
extern void* LIBC_FUNC_NAME(dup3);

extern void* LIBC_FUNC_NAME(dirfd);
extern void* LIBC_FUNC_NAME(opendir);
extern void* LIBC_FUNC_NAME(fdopendir);
extern void* LIBC_FUNC_NAME(readdir);
extern void* LIBC_FUNC_NAME(closedir);

extern void* LIBC_FUNC_NAME(chmod);
extern void* LIBC_FUNC_NAME(fchmod);
extern void* LIBC_FUNC_NAME(fchmodat);

extern void* LIBC_FUNC_NAME(chdir);
extern void* LIBC_FUNC_NAME(fchdir);

extern void* LIBC_FUNC_NAME(getcwd);
extern void* LIBC_FUNC_NAME(get_current_dir_name);

extern void* LIBC_FUNC_NAME(link);
extern void* LIBC_FUNC_NAME(linkat);
extern void* LIBC_FUNC_NAME(symlinkat);

extern void* LIBC_FUNC_NAME(readlinkat);

extern void* LIBC_FUNC_NAME(realpath);


void init_passthrough_if_needed();


#endif //IFS_PASSTHROUGH_HPP
