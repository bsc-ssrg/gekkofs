
#ifndef IFS_METADENTRY_HPP
#define IFS_METADENTRY_HPP

#include <daemon/adafs_daemon.hpp>
#include <global/metadata.hpp>
#include <preload/preload_util.hpp>

int create_node(const std::string& path, const uid_t uid, const gid_t gid, mode_t mode);

void create_metadentry(const std::string& path, Metadata& md);

std::string get_metadentry_str(const std::string& path);

Metadata get_metadentry(const std::string& path);

void remove_node(const std::string& path);

size_t get_metadentry_size(const std::string& path);

void update_metadentry_size(const std::string& path, size_t io_size, off_t offset, bool append);

void update_metadentry(const std::string& path, Metadata& md);

int check_access_mask(const std::string& path, int mask);

std::vector<std::pair<std::string, bool>> get_dirents(const std::string& dir);

#endif //IFS_METADENTRY_HPP
