#include "global/metadata.hpp"
#include "global/configure.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <ctime>
#include <cassert>


static const char MSP = '|'; // metadata separator

Metadata::Metadata(const mode_t mode) :
    atime_(),
    mtime_(),
    ctime_(),
    uid_(),
    gid_(),
    mode_(mode),
    link_count_(0),
    size_(0),
    blocks_(0)
{
    assert(S_ISDIR(mode_) || S_ISREG(mode_));
}

#ifdef HAS_SYMLINKS

Metadata::Metadata(const mode_t mode, const std::string& target_path) :
    atime_(),
    mtime_(),
    ctime_(),
    uid_(),
    gid_(),
    mode_(mode),
    link_count_(0),
    size_(0),
    blocks_(0),
    target_path_(target_path)
{
    assert(S_ISLNK(mode_) || S_ISDIR(mode_) || S_ISREG(mode_));
    // target_path should be there only if this is a link
    assert(target_path_.empty() || S_ISLNK(mode_));
    // target_path should be absolute
    assert(target_path_.empty() || target_path_[0] == '/');
}

#endif

Metadata::Metadata(const std::string& binary_str) {
    size_t read = 0;

    auto ptr = binary_str.data();

    fuid_ = static_cast<fuid_t>(std::stoul(ptr, &read));
    // we read something
    assert(read > 0);
    ptr += read;

    // last parsed char is the separator char
    assert(*ptr == MSP);

    mode_ = static_cast<unsigned int>(std::stoul(++ptr, &read));
    // we read something
    assert(read > 0);
    ptr += read;

    // last parsed char is the separator char
    assert(*ptr == MSP);

    size_ = std::stol(++ptr, &read);
    assert(read > 0);
    ptr += read;

    // The order is important. don't change.
    if (MDATA_USE_ATIME) {
        assert(*ptr == MSP);
        atime_ = static_cast<time_t>(std::stol(++ptr, &read));
        assert(read > 0);
        ptr += read;
    }
    if (MDATA_USE_MTIME) {
        assert(*ptr == MSP);
        mtime_ = static_cast<time_t>(std::stol(++ptr, &read));
        assert(read > 0);
        ptr += read;
    }
    if (MDATA_USE_CTIME) {
        assert(*ptr == MSP);
        ctime_ = static_cast<time_t>(std::stol(++ptr, &read));
        assert(read > 0);
        ptr += read;
    }
    if (MDATA_USE_UID) {
        assert(*ptr == MSP);
        uid_ = static_cast<uid_t>(std::stoul(++ptr, &read));
        assert(read > 0);
        ptr += read;
    }
    if (MDATA_USE_GID) {
        assert(*ptr == MSP);
        gid_ = static_cast<gid_t>(std::stoul(++ptr, &read));
        assert(read > 0);
        ptr += read;
    }
    if (MDATA_USE_LINK_CNT) {
        assert(*ptr == MSP);
        link_count_ = static_cast<nlink_t>(std::stoul(++ptr, &read));
        assert(read > 0);
        ptr += read;
    }
    if (MDATA_USE_BLOCKS) { // last one will not encounter a delimiter anymore
        assert(*ptr == MSP);
        blocks_ = static_cast<blkcnt_t>(std::stoul(++ptr, &read));
        assert(read > 0);
        ptr += read;
    }

#ifdef HAS_SYMLINKS
    // Read target_path
    assert(*ptr == MSP);
    target_path_ = ++ptr;
    // target_path should be there only if this is a link
    assert(target_path_.size() == 0 || S_ISLNK(mode_));
    ptr += target_path_.size();
#endif

    // we consumed all the binary string
    assert(*ptr == '\0');
}

std::string Metadata::serialize() const
{
    std::string s;
    // The order is important. don't change.
    s += std::to_string(fuid_); // add mandatory mode
    s += MSP;
    s += std::to_string(mode_); // add mandatory mode
    s += MSP;
    s += std::to_string(size_); // add mandatory size
    if (MDATA_USE_ATIME) {
        s += MSP;
        s += std::to_string(atime_);
    }
    if (MDATA_USE_MTIME) {
        s += MSP;
        s += std::to_string(mtime_);
    }
    if (MDATA_USE_CTIME) {
        s += MSP;
        s += std::to_string(ctime_);
    }
    if (MDATA_USE_UID) {
        s += MSP;
        s += std::to_string(uid_);
    }
    if (MDATA_USE_GID) {
        s += MSP;
        s += std::to_string(gid_);
    }
    if (MDATA_USE_LINK_CNT) {
        s += MSP;
        s += std::to_string(link_count_);
    }
    if (MDATA_USE_BLOCKS) {
        s += MSP;
        s += std::to_string(blocks_);
    }

#ifdef HAS_SYMLINKS
    s += MSP;
    s += target_path_;
#endif

    return s;
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

//-------------------------------------------- GETTER/SETTER

fuid_t Metadata::fuid() const {
    return fuid_;
}

void Metadata::fuid(fuid_t fuid) {
    fuid_ = fuid;
}

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

#ifdef HAS_SYMLINKS

std::string Metadata::target_path() const {
    assert(!target_path_.empty());
    return target_path_;
}

void Metadata::target_path(const std::string& target_path) {
    // target_path should be there only if this is a link
    assert(target_path.empty() || S_ISLNK(mode_));
    // target_path should be absolute
    assert(target_path.empty() || target_path[0] == '/');
    target_path_ = target_path;
}

bool Metadata::is_link() const {
    return S_ISLNK(mode_);
}

#endif
