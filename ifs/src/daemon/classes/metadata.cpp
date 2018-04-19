
#include <daemon/classes/metadata.hpp>

using namespace std;

static const std::string dentry_val_delim = ","s;

// By default create an empty metadata object

Metadata::Metadata() : atime_(),
                       mtime_(),
                       ctime_(),
                       uid_(),
                       gid_(),
                       mode_(),
                       inode_no_(INVALID_INODE),
                       link_count_(0),
                       size_(0),
                       blocks_(0) {}

Metadata::Metadata(const std::string& path, const mode_t mode) : atime_(),
                                                                 mtime_(),
                                                                 ctime_(),
                                                                 uid_(),
                                                                 gid_(),
                                                                 mode_(mode),
                                                                 inode_no_(INVALID_INODE),
                                                                 link_count_(0),
                                                                 size_(0),
                                                                 blocks_(0),
                                                                 path_(path) {}

/**
 * Creates a metadata object from a value from the database
 * @param path
 * @param db_val
 */
Metadata::Metadata(const std::string& path, std::string db_val) {
    auto pos = db_val.find(dentry_val_delim);
    if (pos == std::string::npos) { // no delimiter found => no metadata enabled. fill with dummy values
        inode_no_ = std::hash<std::string>{}(path);
        mode_ = static_cast<unsigned int>(stoul(db_val));
        link_count_ = 1;
        uid_ = 0;
        gid_ = 0;
        size_ = 0;
        blocks_ = 0;
        atime_ = 0;
        mtime_ = 0;
        ctime_ = 0;
        return;
    }
    // some metadata is enabled: mode is always there
    mode_ = static_cast<unsigned int>(stoul(db_val.substr(0, pos)));
    db_val.erase(0, pos + 1);
    // size is also there
    pos = db_val.find(dentry_val_delim);
    if (pos != std::string::npos) {  // delimiter found. more metadata is coming
        size_ = stol(db_val.substr(0, pos));
        db_val.erase(0, pos + 1);
    } else {
        size_ = stol(db_val);
        return;
    }
    // The order is important. don't change.
    if (ADAFS_DATA->atime_state()) {
        pos = db_val.find(dentry_val_delim);
        atime_ = static_cast<time_t>(stol(db_val.substr(0, pos)));
        db_val.erase(0, pos + 1);
    }
    if (ADAFS_DATA->mtime_state()) {
        pos = db_val.find(dentry_val_delim);
        mtime_ = static_cast<time_t>(stol(db_val.substr(0, pos)));
        db_val.erase(0, pos + 1);
    }
    if (ADAFS_DATA->ctime_state()) {
        pos = db_val.find(dentry_val_delim);
        ctime_ = static_cast<time_t>(stol(db_val.substr(0, pos)));
        db_val.erase(0, pos + 1);
    }
    if (ADAFS_DATA->uid_state()) {
        pos = db_val.find(dentry_val_delim);
        uid_ = static_cast<uid_t>(stoul(db_val.substr(0, pos)));
        db_val.erase(0, pos + 1);
    }
    if (ADAFS_DATA->gid_state()) {
        pos = db_val.find(dentry_val_delim);
        gid_ = static_cast<uid_t>(stoul(db_val.substr(0, pos)));
        db_val.erase(0, pos + 1);
    }
    if (ADAFS_DATA->inode_no_state()) {
        pos = db_val.find(dentry_val_delim);
        inode_no_ = static_cast<ino_t>(stoul(db_val.substr(0, pos)));
        db_val.erase(0, pos + 1);
    }
    if (ADAFS_DATA->link_cnt_state()) {
        pos = db_val.find(dentry_val_delim);
        link_count_ = static_cast<nlink_t>(stoul(db_val.substr(0, pos)));
        db_val.erase(0, pos + 1);
    }
    if (ADAFS_DATA->blocks_state()) { // last one will not encounter a delimiter anymore
        blocks_ = static_cast<blkcnt_t>(stoul(db_val));
    }
}

void Metadata::init_ACM_time() {
    std::time_t time;
    std::time(&time);
    atime_ = time;
    mtime_ = time;
    ctime_ = time;
}

void Metadata::update_ACM_time(bool a, bool c, bool m) {
    std::time_t time;
    std::time(&time);
    if (a)
        atime_ = time;
    if (c)
        ctime_ = time;
    if (m)
        mtime_ = time;
}

/**
 * Creates a key value metadentry string that is used as a value in the KV store
 * @return
 */
std::string Metadata::to_KVentry() const {
    std::string val;
    this->serialize(val);
    return val;
}

void Metadata::serialize(std::string& s) const {
    // The order is important. don't change.
    s += fmt::FormatInt(mode_).c_str(); // add mandatory mode
    s += dentry_val_delim + fmt::FormatInt(size_).c_str(); // add mandatory size
    if (ADAFS_DATA->atime_state()) {
        s += dentry_val_delim + fmt::FormatInt(atime_).c_str();
    }
    if (ADAFS_DATA->mtime_state()) {
        s += dentry_val_delim + fmt::FormatInt(mtime_).c_str();
    }
    if (ADAFS_DATA->ctime_state()) {
        s += dentry_val_delim + fmt::FormatInt(ctime_).c_str();
    }
    if (ADAFS_DATA->uid_state()) {
        s += dentry_val_delim + fmt::FormatInt(uid_).str();
    }
    if (ADAFS_DATA->gid_state()) {
        s += dentry_val_delim + fmt::FormatInt(gid_).str();
    }
    if (ADAFS_DATA->inode_no_state()) {
        s += dentry_val_delim + fmt::FormatInt(inode_no_).str();
    }
    if (ADAFS_DATA->link_cnt_state()) {
        s += dentry_val_delim + fmt::FormatInt(link_count_).c_str();
    }
    if (ADAFS_DATA->blocks_state()) {
        s += dentry_val_delim + fmt::FormatInt(blocks_).c_str();
    }
}

//-------------------------------------------- GETTER/SETTER

time_t Metadata::atime() const {
    return atime_;
}

void Metadata::atime(time_t atime_) {
    Metadata::atime_ = atime_;
}

time_t Metadata::mtime() const {
    return mtime_;
}

void Metadata::mtime(time_t mtime_) {
    Metadata::mtime_ = mtime_;
}

time_t Metadata::ctime() const {
    return ctime_;
}

void Metadata::ctime(time_t ctime_) {
    Metadata::ctime_ = ctime_;
}

uid_t Metadata::uid() const {
    return uid_;
}

void Metadata::uid(uid_t uid_) {
    Metadata::uid_ = uid_;
}

gid_t Metadata::gid() const {
    return gid_;
}

void Metadata::gid(gid_t gid_) {
    Metadata::gid_ = gid_;
}

mode_t Metadata::mode() const {
    return mode_;
}

void Metadata::mode(mode_t mode_) {
    Metadata::mode_ = mode_;
}

uint64_t Metadata::inode_no() const {
    return inode_no_;
}

void Metadata::inode_no(uint64_t inode_no_) {
    Metadata::inode_no_ = inode_no_;
}

nlink_t Metadata::link_count() const {
    return link_count_;
}

void Metadata::link_count(nlink_t link_count_) {
    Metadata::link_count_ = link_count_;
}

size_t Metadata::size() const {
    return size_;
}

void Metadata::size(size_t size_) {
    Metadata::size_ = size_;
}

blkcnt_t Metadata::blocks() const {
    return blocks_;
}

void Metadata::blocks(blkcnt_t blocks_) {
    Metadata::blocks_ = blocks_;
}

const std::string& Metadata::path() const {
    return path_;
}

void Metadata::path(const std::string& path) {
    Metadata::path_ = path;
}
