#include <preload/preload_context.hpp>

#include <preload/open_file_map.hpp>
#include <preload/open_dir.hpp>
#include <preload/resolve.hpp>
#include <global/path_util.hpp>
#include "preload/rpc/engine.hpp"
#include <cassert>


PreloadContext::PreloadContext():
    ofm_(std::make_shared<OpenFileMap>()),
    fs_conf_(std::make_shared<FsConfig>())
{}

void PreloadContext::log(std::shared_ptr<spdlog::logger> logger) {
    log_ = logger;
}

std::shared_ptr<spdlog::logger> PreloadContext::log() const {
    return log_;
}

void PreloadContext::mountdir(const std::string& path) {
    assert(is_absolute_path(path));
    assert(!has_trailing_slash(path));
    mountdir_components_ = split_path(path);
    mountdir_ = path;
}

const std::string& PreloadContext::mountdir() const {
    return mountdir_;
}

void PreloadContext::daemon_addr_str(const std::string& addr) {
    assert(!addr.empty());
    daemon_addr_str_ = addr;
}

const std::string& PreloadContext::daemon_addr_str() const {
    assert(!daemon_addr_str_.empty());
    return daemon_addr_str_;
}

const std::vector<std::string>& PreloadContext::mountdir_components() const {
    return mountdir_components_;
}

void PreloadContext::cwd(const std::string& path) {
    log_->debug("Setting CWD to '{}'", path);
    cwd_ = path;
}

const std::string& PreloadContext::cwd() const {
    return cwd_;
}

RelativizeStatus PreloadContext::relativize_fd_path(int dirfd,
                                                    const char * raw_path,
                                                    std::string& relative_path,
                                                    bool resolve_last_link) const {

    // Relativize path should be called only after the library constructor has been executed
    assert(initialized_);
    // If we run the constructor we also already setup the mountdir
    assert(!mountdir_.empty());

    // We assume raw path is valid
    assert(raw_path != nullptr);

    std::string path;

    if (raw_path[0] != PSP) {
        // path is relative
        if (dirfd == AT_FDCWD) {
            // path is relative to cwd
            path = prepend_path(cwd_, raw_path);
        } else {
            if (!ofm_->exist(dirfd)) {
                return RelativizeStatus::fd_unknown;
            }
            // path is relative to fd
            auto dir = ofm_->get_dir(dirfd);
            if (dir == nullptr) {
                return RelativizeStatus::fd_not_a_dir;
            }
            path = mountdir_;
            path.append(dir->path());
            path.push_back(PSP);
            path.append(raw_path);
        }
    } else {
        path = raw_path;
    }

    if (resolve_path(path, relative_path, resolve_last_link)) {
        return RelativizeStatus::internal;
    }
    return RelativizeStatus::external;
}

bool PreloadContext::relativize_path(const char * raw_path, std::string& relative_path, bool resolve_last_link) const {
    // Relativize path should be called only after the library constructor has been executed
    assert(initialized_);
    // If we run the constructor we also already setup the mountdir
    assert(!mountdir_.empty());

    // We assume raw path is valid
    assert(raw_path != nullptr);

    std::string path;

    if(raw_path[0] != PSP) {
        /* Path is not absolute, we need to prepend CWD;
         * First reserve enough space to minimize memory copy
         */
        path = prepend_path(cwd_, raw_path);
    } else {
        path = raw_path;
    }
    return resolve_path(path, relative_path, resolve_last_link);
}

const std::shared_ptr<OpenFileMap>& PreloadContext::file_map() const {
    return ofm_;
}

void PreloadContext::distributor(std::shared_ptr<Distributor> d) {
    distributor_ = d;
}

std::shared_ptr<Distributor> PreloadContext::distributor() const {
    return distributor_;
}

void PreloadContext::rpc(std::shared_ptr<RPCEngine> rpc) {
    rpc_ = rpc;
}

std::shared_ptr<RPCEngine> PreloadContext::rpc() const {
    return rpc_;
}

const std::shared_ptr<FsConfig>& PreloadContext::fs_conf() const {
    return fs_conf_;
}

void PreloadContext::initialized(const bool& flag) {
    initialized_ = flag;
}

bool PreloadContext::initialized() const {
    return initialized_;
}
