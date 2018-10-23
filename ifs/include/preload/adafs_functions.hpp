#ifndef IFS_ADAFS_FUNCTIONS_HPP
#define IFS_ADAFS_FUNCTIONS_HPP

#include <preload/open_file_map.hpp>
#include <global/metadata.hpp>
/*
 * See include/linux/statfs.h (not includable)
 *
 * Definitions for the flag in f_flag.
 *
 * Generally these flags are equivalent to the MS_ flags used in the mount
 * ABI.  The exception is ST_VALID which has the same value as MS_REMOUNT
 * which doesn't make any sense for statfs.
 */
#define ST_RDONLY    0x0001    /* mount read-only */
#define ST_NOSUID    0x0002    /* ignore suid and sgid bits */
#define ST_NODEV    0x0004    /* disallow access to device special files */
#define ST_NOEXEC    0x0008    /* disallow program execution */
#define ST_SYNCHRONOUS    0x0010    /* writes are synced at once */
#define ST_VALID    0x0020    /* f_flags support is implemented */
#define ST_MANDLOCK    0x0040    /* allow mandatory locks on an FS */
/* 0x0080 used for ST_WRITE in glibc */
/* 0x0100 used for ST_APPEND in glibc */
/* 0x0200 used for ST_IMMUTABLE in glibc */
#define ST_NOATIME    0x0400    /* do not update access times */
#define ST_NODIRATIME    0x0800    /* do not update directory access times */
#define ST_RELATIME    0x1000    /* update atime relative to mtime/ctime */

std::shared_ptr<Metadata> adafs_metadata(const std::string& path);

int adafs_open(const std::string& path, mode_t mode, int flags);

int adafs_mk_node(const std::string& path, mode_t mode, fuid_t& fuid);
int adafs_mk_node(const std::string& path, mode_t mode);

int adafs_rename(const std::string& oldpath, const std::string& newpath);

int adafs_rm_node(const std::string& path);

int adafs_access(const std::string& path, int mask);

int adafs_stat(const std::string& path, struct stat* buf);

int adafs_stat64(const std::string& path, struct stat64* buf);

int adafs_statfs(struct statfs* buf);

off64_t adafs_lseek(int fd, off64_t offset, int whence);

off64_t adafs_lseek(std::shared_ptr<OpenFile> adafs_fd, off64_t offset, int whence);

int adafs_truncate(const std::string& path, off_t offset);

int adafs_truncate(const std::string& path, const fuid_t fuid, off_t old_size, off_t new_size);

int adafs_dup(int oldfd);

int adafs_dup2(int oldfd, int newfd);


ssize_t adafs_pwrite(std::shared_ptr<OpenFile> file,
                     const char * buf, size_t count, off64_t offset);
ssize_t adafs_pwrite_ws(int fd, const void* buf, size_t count, off64_t offset);
ssize_t adafs_write(int fd, const void * buf, size_t count);
ssize_t adafs_pwritev(int fd, const struct iovec *iov, int iovcnt, off_t offset);
ssize_t adafs_writev(int fd, const struct iovec * iov, int iovcnt);

ssize_t adafs_pread(std::shared_ptr<OpenFile> file, char * buf, size_t count, off64_t offset);
ssize_t adafs_pread_ws(int fd, void* buf, size_t count, off64_t offset);
ssize_t adafs_read(int fd, void* buf, size_t count);


int adafs_opendir(const std::string& path);

int getdents(unsigned int fd,
             struct linux_dirent *dirp,
             unsigned int count);

int adafs_rmdir(const std::string& path);

#endif //IFS_ADAFS_FUNCTIONS_HPP
