/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <hermes.hpp>
#include <client/preload_context.hpp>

#include <libsyscall_intercept_hook_point.h>
#include <syscall.h>

#include <client/env.hpp>
#include <global/env_util.hpp>
#include <client/logging.hpp>
#include <client/open_file_map.hpp>
#include <client/open_dir.hpp>
#include <client/resolve.hpp>

#include <global/path_util.hpp>
#include <cassert>
#include <cstdio>


PreloadContext::PreloadContext():
    ofm_(std::make_shared<OpenFileMap>()),
    fs_conf_(std::make_shared<FsConfig>()) {

    internal_fds_.set();
}

void 
PreloadContext::init_logging() {

    const std::string log_opts = 
        gkfs::env::get_var(gkfs::env::LOG, DEFAULT_CLIENT_LOG_LEVEL);

    const std::string log_output = 
        gkfs::env::get_var(gkfs::env::LOG_OUTPUT, DEFAULT_CLIENT_LOG_PATH);

    const std::string trunc_val = 
        gkfs::env::get_var(gkfs::env::LOG_OUTPUT_TRUNC);

    const bool log_trunc = !(!trunc_val.empty() && trunc_val[0] == 0);

    gkfs::log::create_global_logger(log_opts, log_output, log_trunc);
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

const std::vector<std::string>& PreloadContext::mountdir_components() const {
    return mountdir_components_;
}

void PreloadContext::cwd(const std::string& path) {
    cwd_ = path;
}

const std::string& PreloadContext::cwd() const {
    return cwd_;
}

const std::vector<hermes::endpoint>& PreloadContext::hosts() const {
    return hosts_;
}

void PreloadContext::hosts(const std::vector<hermes::endpoint>& endpoints) {
    hosts_ = endpoints;
}

void PreloadContext::clear_hosts() {
    hosts_.clear();
}

uint64_t PreloadContext::local_host_id() const {
    return local_host_id_;
}

void PreloadContext::local_host_id(uint64_t id) {
    local_host_id_ = id;
}

RelativizeStatus PreloadContext::relativize_fd_path(int dirfd,
                                                    const char * raw_path,
                                                    std::string& relative_path,
                                                    bool resolve_last_link) const {

    // Relativize path should be called only after the library constructor has been executed
    assert(interception_enabled_);
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
    assert(interception_enabled_);
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

const std::shared_ptr<FsConfig>& PreloadContext::fs_conf() const {
    return fs_conf_;
}

void PreloadContext::enable_interception() {
    interception_enabled_ = true;
}

void PreloadContext::disable_interception() {
    interception_enabled_ = false;
}

bool PreloadContext::interception_enabled() const {
    return interception_enabled_;
}

int PreloadContext::register_internal_fd(int fd) {

    LOG(DEBUG, "registering fd {} as internal", fd);

    assert(fd >= 0);

    std::lock_guard<std::mutex> lock(internal_fds_mutex_);
    const int pos = internal_fds_._Find_first();
    internal_fds_.reset(pos);


#if !defined(GKFS_DISABLE_LOGGING) && defined(GKFS_DEBUG_BUILD)
    long args[gkfs::syscall::MAX_ARGS]{fd, pos + INTERNAL_FD_BASE, O_CLOEXEC};
#endif

    LOG(SYSCALL, 
        gkfs::syscall::from_internal_code | 
        gkfs::syscall::to_kernel |
        gkfs::syscall::not_executed, 
        SYS_dup3, args);

    const int ifd = 
        ::syscall_no_intercept(SYS_dup3, fd, pos + INTERNAL_FD_BASE, O_CLOEXEC);

    LOG(SYSCALL, 
        gkfs::syscall::from_internal_code | 
        gkfs::syscall::to_kernel |
        gkfs::syscall::executed, 
        SYS_dup3, args, ifd);

    assert(::syscall_error_code(ifd) != -1);

#if !defined(GKFS_DISABLE_LOGGING) && defined(GKFS_DEBUG_BUILD)
    long args2[gkfs::syscall::MAX_ARGS]{fd};
#endif

    LOG(SYSCALL, 
        gkfs::syscall::from_internal_code | 
        gkfs::syscall::to_kernel |
        gkfs::syscall::not_executed, 
        SYS_close, args2);

#if !defined(GKFS_DISABLE_LOGGING) && defined(GKFS_DEBUG_BUILD)
    int rv = ::syscall_no_intercept(SYS_close, fd);
#else
    ::syscall_no_intercept(SYS_close, fd);
#endif

    LOG(SYSCALL, 
        gkfs::syscall::from_internal_code | 
        gkfs::syscall::to_kernel |
        gkfs::syscall::executed, 
        SYS_close, args2, rv);

    LOG(DEBUG, "    (fd {} reassigned to ifd {})", fd, ifd);

    return ifd;
}

void PreloadContext::unregister_internal_fd(int fd) {

    LOG(DEBUG, "unregistering internal fd {}", fd);

    assert(fd >= INTERNAL_FD_BASE);

    const auto pos = fd - INTERNAL_FD_BASE;

    std::lock_guard<std::mutex> lock(internal_fds_mutex_);
    internal_fds_.set(pos);
}

bool PreloadContext::is_internal_fd(int fd) const {

    if(fd < INTERNAL_FD_BASE) {
        return false;
    }

    const auto pos = fd - INTERNAL_FD_BASE;

    std::lock_guard<std::mutex> lock(internal_fds_mutex_);
    return !internal_fds_.test(pos);
}


