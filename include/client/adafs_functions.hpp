#ifndef IFS_ADAFS_FUNCTIONS_HPP
#define IFS_ADAFS_FUNCTIONS_HPP

#include <client/open_file_map.hpp>
#include <global/metadata.hpp>

std::shared_ptr<Metadata> adafs_metadata(const std::string& path, bool follow_links = false);

int adafs_open(const std::string& path, mode_t mode, int flags);

int adafs_mk_node(const std::string& path, mode_t mode, fuid_t& fuid);
int adafs_mk_node(const std::string& path, mode_t mode);

int adafs_rename(const std::string& oldpath, const std::string& newpath);

int adafs_rm_node(const std::string& path);

int adafs_access(const std::string& path, int mask, bool follow_links = true);

int adafs_stat(const std::string& path, struct stat* buf, bool follow_links = true);

int adafs_statvfs(struct statvfs* buf);

int adafs_statfs(struct statfs* buf);

off64_t adafs_lseek(int fd, off64_t offset, int whence);

off64_t adafs_lseek(std::shared_ptr<OpenFile> adafs_fd, off64_t offset, int whence);

int adafs_truncate(const std::string& path, off_t offset);

int adafs_truncate(const std::string& path, const fuid_t fuid, off_t old_size, off_t new_size);

int adafs_dup(int oldfd);

int adafs_dup2(int oldfd, int newfd);

#ifdef HAS_SYMLINKS
int adafs_mk_symlink(const std::string& path, const std::string& target_path);
int adafs_readlink(const std::string& path, char *buf, int bufsize);
#endif


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

struct dirent * adafs_readdir(int fd);

int adafs_rmdir(const std::string& path);

#endif //IFS_ADAFS_FUNCTIONS_HPP
