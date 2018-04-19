
#include <daemon/db/db_ops.hpp>
#include <daemon/db/merge.hpp>

using namespace rocksdb;
using namespace std;

bool db_get_metadentry(const std::string& key, std::string& val) {
    auto db = ADAFS_DATA->rdb();
    auto err = db->Get(ReadOptions(), key, &val).ok();
    // TODO check what happens if nothing could have been found. Will val be NULL, nullptr, ""?
    // It matters because the client RPC is checking for an empty string to see if get_attr was successful or not
    return err;
}

bool db_put_metadentry(const std::string& key, const std::string& val) {
    auto db = ADAFS_DATA->rdb();
    auto cop = CreateOperand(val);
    auto s = db->Merge(ADAFS_DATA->rdb_write_options(), key, cop.serialize());
    if(!s.ok()){
        ADAFS_DATA->spdlogger()->error("Failed to create metadentry size. RDB error: [{}]", s.ToString());
    }
    return s.ok();
}

bool db_delete_metadentry(const std::string& key) {
    auto db = ADAFS_DATA->rdb();
    return db->Delete(ADAFS_DATA->rdb_write_options(), key).ok();
}

bool db_metadentry_exists(const std::string& key) {
    auto db = ADAFS_DATA->rdb();
    string val_str;
    return db->Get(ReadOptions(), key, &val_str).ok();
}

bool db_is_dir_entry(const std::string& dir_path) {
    // TODO
    return true;
}

/**
 * Updates a metadentry atomically and also allows to change keys
 * @param old_key
 * @param new_key
 * @param val
 * @return
 */
bool db_update_metadentry(const std::string& old_key, const std::string& new_key, const std::string& val) {
    auto db = ADAFS_DATA->rdb();
    rocksdb::WriteBatch batch;
    batch.Delete(old_key);
    batch.Put(new_key, val);
    return db->Write(ADAFS_DATA->rdb_write_options(), &batch).ok();
}

bool db_update_metadentry_size(const std::string& key,
        size_t size, off64_t offset, bool append) {
    auto db = ADAFS_DATA->rdb();
    auto uop = IncreaseSizeOperand(offset + size, append);
    auto s = db->Merge(ADAFS_DATA->rdb_write_options(), key, uop.serialize());
    if(!s.ok()){
        ADAFS_DATA->spdlogger()->error("Failed to update metadentry size. RDB error: [{}]", s.ToString());
    }
    return s.ok();
}

void db_iterate_all_entries() {
    string key;
    string val;
    auto db = ADAFS_DATA->rdb();
    // Do RangeScan on parent inode
    auto iter = db->NewIterator(rocksdb::ReadOptions());
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        key = iter->key().ToString();
        val = iter->value().ToString();
        ADAFS_DATA->spdlogger()->trace("key '{}' value '{}'", key, val);
    }
}