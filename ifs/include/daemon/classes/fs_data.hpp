
#ifndef LFS_FS_DATA_H
#define LFS_FS_DATA_H

#include <daemon/adafs_daemon.hpp>

/* Forward declarations */
class MetadataDB;
class ChunkStorage;
class Distributor;

#include <unordered_map>
#include <map>
#include <functional> //std::hash

class FsData {

private:
    FsData() {}

    // Caching
    std::unordered_map<std::string, std::string> hashmap_;
    std::hash<std::string> hashf_;

    // Later the blocksize will likely be coupled to the chunks to allow individually big chunk sizes.
    blksize_t blocksize_;

    //logger
    std::shared_ptr<spdlog::logger> spdlogger_;

    // paths
    std::string rootdir_;
    std::string mountdir_;
    std::string metadir_;

    // hosts_
    std::string hosts_raw_; // raw hosts string, given when daemon is started. Used to give it to fs client
    std::map<uint64_t, std::string> hosts_;
    uint64_t host_id_; // my host number
    size_t host_size_;
    std::string rpc_port_;

    // Database
    std::shared_ptr<MetadataDB> mdb_;
    // Storage backend
    std::shared_ptr<ChunkStorage> storage_;
    // Distributor
    std::shared_ptr<Distributor> distributor_;

    // configurable metadata
    bool atime_state_;
    bool mtime_state_;
    bool ctime_state_;
    bool uid_state_;
    bool gid_state_;
    bool link_cnt_state_;
    bool blocks_state_;

public:
    static FsData* getInstance() {
        static FsData instance;
        return &instance;
    }

    FsData(FsData const&) = delete;

    void operator=(FsData const&) = delete;

    // Utility member functions

    bool is_local_op(size_t recipient);

    // getter/setter

    const std::unordered_map<std::string, std::string>& hashmap() const;

    void hashmap(const std::unordered_map<std::string, std::string>& hashmap_);

    const std::hash<std::string>& hashf() const;

    void hashf(const std::hash<std::string>& hashf_);

    blksize_t blocksize() const;

    void blocksize(blksize_t blocksize_);

    const std::shared_ptr<spdlog::logger>& spdlogger() const;

    void spdlogger(const std::shared_ptr<spdlog::logger>& spdlogger_);

    const std::string& rootdir() const;

    void rootdir(const std::string& rootdir_);

    const std::string& mountdir() const;

    void mountdir(const std::string& mountdir_);

    const std::string& metadir() const;

    void metadir(const std::string& metadir_);

    const std::shared_ptr<MetadataDB>& mdb() const;

    void mdb(const std::shared_ptr<MetadataDB>& mdb);

    void close_mdb();

    const std::shared_ptr<ChunkStorage>& storage() const;

    void storage(const std::shared_ptr<ChunkStorage>& storage);

    void distributor(std::shared_ptr<Distributor> d);

    std::shared_ptr<Distributor> distributor() const;

    const std::string& hosts_raw() const;

    void hosts_raw(const std::string& hosts_raw);

    const std::map<uint64_t, std::string>& hosts() const;

    void hosts(const std::map<uint64_t, std::string>& hosts);

    const uint64_t& host_id() const;

    void host_id(const uint64_t& host_id);

    size_t host_size() const;

    void host_size(size_t host_size);

    std::string rpc_port() const;

    void rpc_port(std::string rpc_port);

    bool atime_state() const;

    void atime_state(bool atime_state);

    bool mtime_state() const;

    void mtime_state(bool mtime_state);

    bool ctime_state() const;

    void ctime_state(bool ctime_state);

    bool uid_state() const;

    void uid_state(bool uid_state);

    bool gid_state() const;

    void gid_state(bool gid_state);

    bool link_cnt_state() const;

    void link_cnt_state(bool link_cnt_state);

    bool blocks_state() const;

    void blocks_state(bool blocks_state);

};


#endif //LFS_FS_DATA_H
