#include <spdlog/spdlog.h>

#include <daemon/classes/fs_data.hpp>
#include <daemon/backend/metadata/db.hpp>


// Utility member functions

ino_t FsData::raise_inode_count(ino_t count) {
    FsData::inode_count_ += count;
    return FsData::inode_count_;
}

bool FsData::is_local_op(const size_t recipient) {
    return recipient == host_id_;
}

// getter/setter

const std::unordered_map<std::string, std::string>& FsData::hashmap() const {
    return hashmap_;
}

void FsData::hashmap(const std::unordered_map<std::string, std::string>& hashmap_) {
    FsData::hashmap_ = hashmap_;
}

const std::hash<std::string>& FsData::hashf() const {
    return hashf_;
}

void FsData::hashf(const std::hash<std::string>& hashf_) {
    FsData::hashf_ = hashf_;
}

blksize_t FsData::blocksize() const {
    return blocksize_;
}

void FsData::blocksize(blksize_t blocksize_) {
    FsData::blocksize_ = blocksize_;
}

const std::shared_ptr<spdlog::logger>& FsData::spdlogger() const {
    return spdlogger_;
}

void FsData::spdlogger(const std::shared_ptr<spdlog::logger>& spdlogger_) {
    FsData::spdlogger_ = spdlogger_;
}

const std::shared_ptr<MetadataDB>& FsData::mdb() const {
    return mdb_;
}

void FsData::mdb(const std::shared_ptr<MetadataDB>& mdb) {
    mdb_ = mdb;
}

void FsData::close_mdb() {
    mdb_.reset();
}

const std::shared_ptr<ChunkStorage>& FsData::storage() const {
    return storage_;
}

void FsData::storage(const std::shared_ptr<ChunkStorage>& storage) {
    storage_ = storage;
}

void FsData::distributor(std::shared_ptr<Distributor> d) {
    distributor_ = d;
}

std::shared_ptr<Distributor> FsData::distributor() const {
    return distributor_;
}

const std::string& FsData::rootdir() const {
    return rootdir_;
}

void FsData::rootdir(const std::string& rootdir_) {
    FsData::rootdir_ = rootdir_;
}

const std::string& FsData::mountdir() const {
    return mountdir_;
}

void FsData::mountdir(const std::string& mountdir) {
    FsData::mountdir_ = mountdir;
}

const std::string& FsData::metadir() const {
    return metadir_;
}

void FsData::metadir(const std::string& metadir) {
    FsData::metadir_ = metadir;
}

ino_t FsData::inode_count() const {
    return inode_count_;
}

void FsData::inode_count(ino_t inode_count) {
    FsData::inode_count_ = inode_count;
}

const std::string& FsData::hosts_raw() const {
    return hosts_raw_;
}

void FsData::hosts_raw(const std::string& hosts_raw) {
    FsData::hosts_raw_ = hosts_raw;
}

const std::map<uint64_t, std::string>& FsData::hosts() const {
    return hosts_;
}

void FsData::hosts(const std::map<uint64_t, std::string>& hosts) {
    FsData::hosts_ = hosts;
}

const uint64_t& FsData::host_id() const {
    return host_id_;
}

void FsData::host_id(const uint64_t& host_id) {
    FsData::host_id_ = host_id;
}

size_t FsData::host_size() const {
    return host_size_;
}

void FsData::host_size(size_t host_size) {
    FsData::host_size_ = host_size;
}

std::string FsData::rpc_port() const {
    return rpc_port_;
}

void FsData::rpc_port(std::string rpc_port) {
    FsData::rpc_port_ = rpc_port;
}

bool FsData::atime_state() const {
    return atime_state_;
}

void FsData::atime_state(bool atime_state) {
    FsData::atime_state_ = atime_state;
}

bool FsData::mtime_state() const {
    return mtime_state_;
}

void FsData::mtime_state(bool mtime_state) {
    FsData::mtime_state_ = mtime_state;
}

bool FsData::ctime_state() const {
    return ctime_state_;
}

void FsData::ctime_state(bool ctime_state) {
    FsData::ctime_state_ = ctime_state;
}

bool FsData::uid_state() const {
    return uid_state_;
}

void FsData::uid_state(bool uid_state) {
    FsData::uid_state_ = uid_state;
}

bool FsData::gid_state() const {
    return gid_state_;
}

void FsData::gid_state(bool gid_state) {
    FsData::gid_state_ = gid_state;
}

bool FsData::inode_no_state() const {
    return inode_no_state_;
}

void FsData::inode_no_state(bool inode_no_state) {
    FsData::inode_no_state_ = inode_no_state;
}

bool FsData::link_cnt_state() const {
    return link_cnt_state_;
}

void FsData::link_cnt_state(bool link_cnt_state) {
    FsData::link_cnt_state_ = link_cnt_state;
}

bool FsData::blocks_state() const {
    return blocks_state_;
}

void FsData::blocks_state(bool blocks_state) {
    FsData::blocks_state_ = blocks_state;
}








