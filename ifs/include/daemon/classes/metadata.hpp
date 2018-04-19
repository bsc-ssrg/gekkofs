
#ifndef FS_METADATA_H
#define FS_METADATA_H

#include <daemon/adafs_daemon.hpp>

class Metadata {

private:
    time_t atime_;             // access time. gets updated on file access unless mounted with noatime
    time_t mtime_;             // modify time. gets updated when file content is modified.
    time_t ctime_;             // change time. gets updated when the file attributes are changed AND when file content is modified.
    uid_t uid_;
    gid_t gid_;
    mode_t mode_;
    uint64_t inode_no_;
    nlink_t link_count_;       // number of names for this inode (hardlinks)
    size_t size_;               // size_ in bytes, might be computed instead of stored
    blkcnt_t blocks_;          // allocated file system blocks_

    std::string path_;

public:
    Metadata();

    Metadata(const std::string& path, mode_t mode);

    Metadata(const std::string& path, std::string db_val);

    void init_ACM_time();

    void update_ACM_time(bool a, bool c, bool m);

    std::string to_KVentry() const;

    void serialize(std::string& s) const;

    //Getter and Setter
    time_t atime() const;

    void atime(time_t atime_);

    time_t mtime() const;

    void mtime(time_t mtime_);

    time_t ctime() const;

    void ctime(time_t ctime_);

    uid_t uid() const;

    void uid(uid_t uid_);

    gid_t gid() const;

    void gid(gid_t gid_);

    mode_t mode() const;

    void mode(mode_t mode_);

    uint64_t inode_no() const;

    void inode_no(uint64_t inode_no_);

    nlink_t link_count() const;

    void link_count(nlink_t link_count_);

    size_t size() const;

    void size(size_t size_);

    blkcnt_t blocks() const;

    void blocks(blkcnt_t blocks_);

    const std::string& path() const;

    void path(const std::string& path);

};


#endif //FS_METADATA_H
