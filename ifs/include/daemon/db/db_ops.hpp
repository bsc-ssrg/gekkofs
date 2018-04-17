
#ifndef IFS_DB_OPS_HPP
#define IFS_DB_OPS_HPP

#include <daemon/adafs_daemon.hpp>

bool db_get_metadentry(const std::string& key, std::string& val);

bool db_put_metadentry(const std::string& key, const std::string& val);

bool db_delete_metadentry(const std::string& key);

bool db_metadentry_exists(const std::string& key);

bool db_is_dir_entry(const std::string& dir_path);

bool db_update_metadentry(const std::string& old_key, const std::string& new_key, const std::string& val);

bool db_update_metadentry_size(const std::string& key,
        size_t size, off64_t offset, bool append);

void db_iterate_all_entries();

#endif //IFS_DB_OPS_HPP
