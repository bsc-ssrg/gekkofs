//
// Created by evie on 7/21/17.
//

#include <preload/preload.hpp>
#include <preload/ipc_types.hpp>
#include <preload/margo_ipc.hpp>
#include <preload/rpc/ld_rpc_metadentry.hpp>
#include <extern/lrucache/LRUCache11.hpp>

#include <dlfcn.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>
#include <atomic>
#include <cassert>
#include <preload/rpc/ld_rpc_data.hpp>

// TODO my god... someone clean up this mess of a file :_:

static pthread_once_t init_lib_thread = PTHREAD_ONCE_INIT;

// Mercury/Margo IPC Client
margo_instance_id margo_ipc_id_;
hg_addr_t daemon_svr_addr_ = HG_ADDR_NULL;
// Mercury/Margo RPC Client
margo_instance_id margo_rpc_id_;

// IPC IDs
static hg_id_t minimal_id;
static hg_id_t ipc_config_id;
static hg_id_t ipc_open_id;
static hg_id_t ipc_stat_id;
static hg_id_t ipc_unlink_id;
static hg_id_t ipc_update_metadentry_id;
static hg_id_t ipc_update_metadentry_size_id;
static hg_id_t ipc_write_data_id;
static hg_id_t ipc_read_data_id;
// RPC IDs
static hg_id_t rpc_minimal_id;
static hg_id_t rpc_create_node_id;
static hg_id_t rpc_attr_id;
static hg_id_t rpc_remove_node_id;
static hg_id_t rpc_update_metadentry_id;
static hg_id_t rpc_update_metadentry_size_id;
static hg_id_t rpc_write_data_id;
static hg_id_t rpc_read_data_id;
// rpc address cache
typedef lru11::Cache<uint64_t, hg_addr_t> KVCache;
KVCache rpc_address_cache_{32768, 4096}; // XXX Set values are not based on anything...

// misc
static std::atomic<bool> is_env_initialized(false);

// external variables
FILE* debug_fd;
shared_ptr<FsConfig> fs_config;

// function pointer for preloading
void* libc;

void* libc_open;
//void* libc_open64; //unused
void* libc_fopen; // XXX Does not work with streaming pointers. If used will block forever
void* libc_fopen64; // XXX Does not work with streaming pointers. If used will block forever

//void* libc_creat; //unused
//void* libc_creat64; //unused
void* libc_unlink;

void* libc_close;
//void* libc___close; //unused

void* libc_stat;
void* libc_fstat;
void* libc___xstat;
void* libc___xstat64;
void* libc___fxstat;
void* libc___fxstat64;
void* libc___lxstat;
void* libc___lxstat64;

void* libc_access;

void* libc_puts;

void* libc_write;
void* libc_pwrite;
void* libc_read;
void* libc_pread;
void* libc_pread64;

void* libc_lseek;
//void* libc_lseek64; //unused

void* libc_truncate;
void* libc_ftruncate;

void* libc_dup;
void* libc_dup2;


static OpenFileMap file_map{};

int open(const char* path, int flags, ...) {
    init_passthrough_if_needed();
    LD_LOG_DEBUG(debug_fd, "open called with path %s\n", path);
    mode_t mode;
    if (flags & O_CREAT) {
        va_list vl;
        va_start(vl, flags);
        mode = va_arg(vl, int);
        va_end(vl);
    }
    if (is_env_initialized && is_fs_path(path)) {
        auto err = 1;
        auto fd = file_map.add(path, (flags & O_APPEND) != 0);
        if (flags & O_CREAT) { // do file create TODO handle all other flags
            if (fs_config->host_size > 1) { // multiple node operation
                auto recipient = get_rpc_node(path);
                if (is_local_op(recipient)) { // local
                    err = ipc_send_open(path, flags, mode, ipc_open_id);
                } else { // remote
                    err = rpc_send_create_node(rpc_create_node_id, recipient, path,
                                               mode);
                }
            } else { // single node operationHG_Destroy
                err = ipc_send_open(path, flags, mode, ipc_open_id);
            }
        } else {
            // TODO look up if file exists
            err = 0;
        }
        if (err == 0)
            return fd;
        else {
            file_map.remove(fd);
            return -1;
        }
    }
    return (reinterpret_cast<decltype(&open)>(libc_open))(path, flags, mode);
}

int open64(__const char* path, int flags, ...) {
    init_passthrough_if_needed();
    LD_LOG_DEBUG(debug_fd, "open64 called with path %s\n", path);
    mode_t mode;
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = va_arg(ap, mode_t);
        va_end(ap);
    }
    return open(path, flags | O_LARGEFILE, mode);
}

//// TODO This function somehow always blocks forever if one puts anything between the paththru...
//FILE* fopen(const char* path, const char* mode) {
////    init_passthrough_if_needed();
////    DAEMON_DEBUG(debug_fd, "fopen called with path %s\n", path);
//    return (reinterpret_cast<decltype(&fopen)>(libc_fopen))(path, mode);
//}

//// TODO This function somehow always blocks forever if one puts anything between the paththru...
//FILE* fopen64(const char* path, const char* mode) {
////    init_passthrough_if_needed();
////    DAEMON_DEBUG(debug_fd, "fopen64 called with path %s\n", path);
//    return (reinterpret_cast<decltype(&fopen)>(libc_fopen))(path, mode);
//}

#undef creat
int creat(const char* path, mode_t mode) {
    init_passthrough_if_needed();
    LD_LOG_DEBUG(debug_fd, "creat called with path %s with mode %d\n", path, mode);
    return open(path, O_CREAT | O_WRONLY | O_TRUNC, mode);
}

#undef creat64

int creat64(const char* path, mode_t mode) {
    init_passthrough_if_needed();
    LD_LOG_DEBUG(debug_fd, "creat64 called with path %s with mode %d\n", path, mode);
    return open(path, O_CREAT | O_WRONLY | O_TRUNC | O_LARGEFILE, mode);
}

int unlink(const char* path) __THROW {
    init_passthrough_if_needed();
    int err;
//    LD_LOG_DEBUG(debug_fd, "unlink called with path %s\n", path);
    if (is_env_initialized && is_fs_path(path)) {
        if (fs_config->host_size > 1) { // multiple node operation
            auto recipient = get_rpc_node(path);
            if (is_local_op(recipient)) { // local
                err = ipc_send_unlink(path, ipc_unlink_id);
            } else { // remote
                err = rpc_send_remove_node(rpc_remove_node_id, recipient, path);
            }
        } else { // single node operation
            err = ipc_send_unlink(path, ipc_unlink_id);
        }

        return err;
    }
    return (reinterpret_cast<decltype(&unlink)>(libc_unlink))(path);
}

int close(int fd) {
    init_passthrough_if_needed();
    if (is_env_initialized && file_map.exist(fd)) {
        // Currently no call to the daemon is required
        file_map.remove(fd);
        return 0;
    }
    return (reinterpret_cast<decltype(&close)>(libc_close))(fd);
}

int __close(int fd) {
    return close(fd);
}

// TODO combine adafs_stat and adafs_stat64
int adafs_stat(const std::string& path, struct stat* buf) {
    int err;
    string attr = ""s;
    if (fs_config->host_size > 1) { // multiple node operation
        auto recipient = get_rpc_node(path);
        if (is_local_op(recipient)) { // local
            err = ipc_send_stat(path, attr, ipc_stat_id);
        } else { // remote
            err = rpc_send_get_attr(rpc_attr_id, recipient, path, attr);
        }
    } else { // single node operation
        err = ipc_send_stat(path, attr, ipc_stat_id);
    }

    db_val_to_stat(path, attr, *buf);

    return err;
}

int adafs_stat64(const std::string& path, struct stat64* buf) {
    int err;
    string attr = ""s;
    if (fs_config->host_size > 1) { // multiple node operation
        auto recipient = get_rpc_node(path);
        if (is_local_op(recipient)) { // local
            err = ipc_send_stat(path, attr, ipc_stat_id);
        } else { // remote
            err = rpc_send_get_attr(rpc_attr_id, recipient, path, attr);
        }
    } else { // single node operation
        err = ipc_send_stat(path, attr, ipc_stat_id);
    }
    db_val_to_stat64(path, attr, *buf);
    return err;
}

int stat(const char* path, struct stat* buf) __THROW {
    init_passthrough_if_needed();
    LD_LOG_DEBUG(debug_fd, "stat called with path %s\n", path);
    if (is_env_initialized && is_fs_path(path)) {
        // TODO call daemon and return
        return adafs_stat(path, buf);
    }
    return (reinterpret_cast<decltype(&stat)>(libc_stat))(path, buf);
}

int fstat(int fd, struct stat* buf) __THROW {
    init_passthrough_if_needed();
    LD_LOG_DEBUG(debug_fd, "fstat called with fd %d\n", fd);
    if (is_env_initialized && file_map.exist(fd)) {
        auto path = file_map.get(fd)->path(); // TODO use this to send to the daemon (call directly)
        // TODO call daemon and return
        return adafs_stat(path, buf);
    }
    return (reinterpret_cast<decltype(&fstat)>(libc_fstat))(fd, buf);
}

int __xstat(int ver, const char* path, struct stat* buf) __THROW {
    init_passthrough_if_needed();
    LD_LOG_DEBUG(debug_fd, "__xstat called with path %s\n", path);
    if (is_env_initialized && is_fs_path(path)) {
        // TODO call stat
        return adafs_stat(path, buf);
    }
    return (reinterpret_cast<decltype(&__xstat)>(libc___xstat))(ver, path, buf);
}

int __xstat64(int ver, const char* path, struct stat64* buf) __THROW {
    init_passthrough_if_needed();
    LD_LOG_DEBUG(debug_fd, "__xstat64 called with path %s\n", path);
    if (is_env_initialized && is_fs_path(path)) {
        return adafs_stat64(path, buf);
//        // Not implemented
//        return -1;
    }
    return (reinterpret_cast<decltype(&__xstat64)>(libc___xstat64))(ver, path, buf);
}

int __fxstat(int ver, int fd, struct stat* buf) __THROW {
    init_passthrough_if_needed();
    LD_LOG_DEBUG(debug_fd, "__fxstat called with fd %d\n", fd);
    if (is_env_initialized && file_map.exist(fd)) {
        // TODO call fstat
        auto path = file_map.get(fd)->path();
        return adafs_stat(path, buf);
    }
    return (reinterpret_cast<decltype(&__fxstat)>(libc___fxstat))(ver, fd, buf);
}

int __fxstat64(int ver, int fd, struct stat64* buf) __THROW {
    init_passthrough_if_needed();
    LD_LOG_DEBUG(debug_fd, "__fxstat64 called with fd %d\n", fd);
    if (is_env_initialized && file_map.exist(fd)) {
        // TODO call fstat64
        auto path = file_map.get(fd)->path();
        return adafs_stat64(path, buf);
    }
    return (reinterpret_cast<decltype(&__fxstat64)>(libc___fxstat64))(ver, fd, buf);
}

extern int __lxstat(int ver, const char* path, struct stat* buf) __THROW {
    init_passthrough_if_needed();
    if (is_env_initialized && is_fs_path(path)) {
        // Not implemented
        return -1;
    }
    return (reinterpret_cast<decltype(&__lxstat)>(libc___lxstat))(ver, path, buf);
}

extern int __lxstat64(int ver, const char* path, struct stat64* buf) __THROW {
    init_passthrough_if_needed();
    if (is_env_initialized && is_fs_path(path)) {
        // Not implemented
        return -1;
    }
    return (reinterpret_cast<decltype(&__lxstat64)>(libc___lxstat64))(ver, path, buf);
}

int access(const char* path, int mode) __THROW {
    init_passthrough_if_needed();
    if (is_env_initialized && is_fs_path(path)) {
        // TODO
    }
    return (reinterpret_cast<decltype(&access)>(libc_access))(path, mode);
}

int puts(const char* str) {
    return (reinterpret_cast<decltype(&puts)>(libc_puts))(str);
}

ssize_t write(int fd, const void* buf, size_t count) {
    init_passthrough_if_needed();
    if (is_env_initialized && file_map.exist(fd)) {
        // TODO if append flag has been given, set offset accordingly.
        // XXX handle lseek too
        return pwrite(fd, buf, count, 0);
    }
    return (reinterpret_cast<decltype(&write)>(libc_write))(fd, buf, count);
}

ssize_t pwrite(int fd, const void* buf, size_t count, off_t offset) {
    init_passthrough_if_needed();
    if (is_env_initialized && file_map.exist(fd)) {
        auto adafs_fd = file_map.get(fd);
        auto path = adafs_fd->path();
        auto append_flag = adafs_fd->append_flag();
        size_t write_size;
        int err = 0;
        long updated_size = 0;
        /*
         * Update the metadentry size first to prevent two processes to write to the same offset when O_APPEND is given.
         * The metadentry size update is atomic XXX actually not yet. see metadentry.cpp
         */
//        if (append_flag)
            err = rpc_send_update_metadentry_size(ipc_update_metadentry_size_id, rpc_update_metadentry_size_id, path,
                                                  count, append_flag, updated_size);
        if (err != 0) {
            LD_LOG_ERROR0(debug_fd, "pwrite: update_metadentry_size failed\n");
            return 0; // ERR
        }
        err = rpc_send_write(ipc_write_data_id, rpc_write_data_id, path, count, offset, buf, write_size, append_flag,
                             updated_size);
        if (err != 0) {
            LD_LOG_ERROR0(debug_fd, "pwrite: write failed\n");
            return 0;
        }
        return write_size;
    }
    return (reinterpret_cast<decltype(&pwrite)>(libc_pwrite))(fd, buf, count, offset);
}

ssize_t read(int fd, void* buf, size_t count) {
    init_passthrough_if_needed();
    if (is_env_initialized && file_map.exist(fd)) {
        return pread(fd, buf, count, 0);
    }
    return (reinterpret_cast<decltype(&read)>(libc_read))(fd, buf, count);
}

ssize_t pread(int fd, void* buf, size_t count, off_t offset) {
    init_passthrough_if_needed();
    if (is_env_initialized && file_map.exist(fd)) {
        auto adafs_fd = file_map.get(fd);
        auto path = adafs_fd->path();
        size_t read_size = 0;
        int err;
        err = rpc_send_read(ipc_read_data_id, rpc_read_data_id, path, count, offset, buf, read_size);
        // TODO check how much we need to deal with the read_size
        return err == 0 ? read_size : 0;
    }
    return (reinterpret_cast<decltype(&pread)>(libc_pread))(fd, buf, count, offset);
}

ssize_t pread64(int fd, void* buf, size_t nbyte, __off64_t offset) {
    init_passthrough_if_needed();
    if (is_env_initialized && file_map.exist(fd)) {
//        auto path = file_map.get(fd)->path(); // TODO use this to send to the daemon (call directly)
        // TODO call daemon and return size written
        return 0; // TODO
    }
    return (reinterpret_cast<decltype(&pread64)>(libc_pread64))(fd, buf, nbyte, offset);
}

off_t lseek(int fd, off_t offset, int whence) __THROW {
    init_passthrough_if_needed();
    return (reinterpret_cast<decltype(&lseek)>(libc_lseek))(fd, offset, whence);
}

off_t lseek64(int fd, off_t offset, int whence) __THROW {
    init_passthrough_if_needed();
    return lseek(fd, offset, whence);
}

int truncate(const char* path, off_t length) __THROW {
    init_passthrough_if_needed();
    return (reinterpret_cast<decltype(&truncate)>(libc_truncate))(path, length);
}

int ftruncate(int fd, off_t length) __THROW {
    init_passthrough_if_needed();
    return (reinterpret_cast<decltype(&ftruncate)>(libc_ftruncate))(fd, length);
}

int dup(int oldfd) __THROW {
    init_passthrough_if_needed();
    if (is_env_initialized && file_map.exist(oldfd)) {
        // Not implemented
        return -1;
    }
    return (reinterpret_cast<decltype(&dup)>(libc_dup))(oldfd);
}

int dup2(int oldfd, int newfd) __THROW {
    init_passthrough_if_needed();
    if (is_env_initialized && (file_map.exist(oldfd) || file_map.exist(newfd))) {
        // Not implemented
        return -1;
    }
    return (reinterpret_cast<decltype(&dup2)>(libc_dup2))(oldfd, newfd);
}

bool get_addr_by_hostid(const uint64_t hostid, hg_addr_t& svr_addr) {

    if (rpc_address_cache_.tryGet(hostid, svr_addr)) {
        LD_LOG_TRACE0(debug_fd, "tryGet successful and put in svr_addr\n");
        //found
        return true;
    } else {
        LD_LOG_TRACE0(debug_fd, "not found in lrucache\n");
        // not found, manual lookup and add address mapping to LRU cache
        auto hostname = RPC_PROTOCOL + "://"s + fs_config->hosts.at(hostid) + ":"s +
                        fs_config->rpc_port; // convert hostid to hostname and port
        LD_LOG_TRACE(debug_fd, "generated hostname %s with rpc_port %s\n", hostname.c_str(),
                     fs_config->rpc_port.c_str());
        margo_addr_lookup(margo_rpc_id_, hostname.c_str(), &svr_addr);
        if (svr_addr == HG_ADDR_NULL)
            return false;
        rpc_address_cache_.insert(hostid, svr_addr);
        return true;
    }
}

size_t get_rpc_node(const string& to_hash) {
    return std::hash<string>{}(to_hash) % fs_config->host_size;
}

bool is_local_op(const size_t recipient) {
    return recipient == fs_config->host_id;
}

void register_client_ipcs(margo_instance_id mid) {
    minimal_id = MARGO_REGISTER(mid, "rpc_minimal", rpc_minimal_in_t, rpc_minimal_out_t, NULL);
    ipc_open_id = MARGO_REGISTER(mid, "ipc_srv_open", ipc_open_in_t, ipc_err_out_t, NULL);
    ipc_stat_id = MARGO_REGISTER(mid, "ipc_srv_stat", ipc_stat_in_t, ipc_stat_out_t, NULL);
    ipc_unlink_id = MARGO_REGISTER(mid, "ipc_srv_unlink", ipc_unlink_in_t, ipc_err_out_t, NULL);
    ipc_update_metadentry_id = MARGO_REGISTER(mid, "rpc_srv_update_metadentry", rpc_update_metadentry_in_t,
                                              rpc_err_out_t, NULL);
    ipc_update_metadentry_size_id = MARGO_REGISTER(mid, "rpc_srv_update_metadentry_size",
                                                   rpc_update_metadentry_size_in_t, rpc_update_metadentry_size_out_t,
                                                   NULL);
    ipc_config_id = MARGO_REGISTER(mid, "ipc_srv_fs_config", ipc_config_in_t, ipc_config_out_t,
                                   NULL);
    ipc_write_data_id = MARGO_REGISTER(mid, "rpc_srv_write_data", rpc_write_data_in_t, rpc_data_out_t,
                                       NULL);
    ipc_read_data_id = MARGO_REGISTER(mid, "rpc_srv_read_data", rpc_read_data_in_t, rpc_data_out_t,
                                      NULL);
}

void register_client_rpcs(margo_instance_id mid) {
    rpc_minimal_id = MARGO_REGISTER(mid, "rpc_minimal", rpc_minimal_in_t, rpc_minimal_out_t, NULL);
    rpc_create_node_id = MARGO_REGISTER(mid, "rpc_srv_create_node", rpc_create_node_in_t,
                                        rpc_err_out_t, NULL);
    rpc_attr_id = MARGO_REGISTER(mid, "rpc_srv_attr", rpc_get_attr_in_t, rpc_get_attr_out_t, NULL);
    rpc_remove_node_id = MARGO_REGISTER(mid, "rpc_srv_remove_node", rpc_remove_node_in_t,
                                        rpc_err_out_t, NULL);
    rpc_update_metadentry_id = MARGO_REGISTER(mid, "rpc_srv_update_metadentry", rpc_update_metadentry_in_t,
                                              rpc_err_out_t, NULL);
    rpc_update_metadentry_size_id = MARGO_REGISTER(mid, "rpc_srv_update_metadentry_size",
                                                   rpc_update_metadentry_size_in_t, rpc_update_metadentry_size_out_t,
                                                   NULL);
    rpc_write_data_id = MARGO_REGISTER(mid, "rpc_srv_write_data", rpc_write_data_in_t, rpc_data_out_t,
                                       NULL);
    rpc_read_data_id = MARGO_REGISTER(mid, "rpc_srv_read_data", rpc_read_data_in_t, rpc_data_out_t,
                                      NULL);
}

bool init_ipc_client() {
    auto protocol_port = "na+sm"s;
    LD_LOG_DEBUG0(debug_fd, "Initializing Margo IPC client ...\n");

    // Start Margo (this will also initialize Argobots and Mercury internally)
    auto mid = margo_init(protocol_port.c_str(), MARGO_CLIENT_MODE, 1, -1);

    if (mid == MARGO_INSTANCE_NULL) {
        LD_LOG_DEBUG0(debug_fd, "[ERR]: margo_init failed to initialize the Margo IPC client\n");
        return false;
    }
    LD_LOG_DEBUG0(debug_fd, "Success.\n");


    margo_ipc_id_ = mid;
//    margo_diag_start(mid);

    auto adafs_daemon_pid = getProcIdByName("adafs_daemon"s);
    if (adafs_daemon_pid == -1) {
        printf("[ERR] ADA-FS daemon not started. Exiting ...\n");
        return false;
    }
    printf("[INFO] ADA-FS daemon with PID %d found.\n", adafs_daemon_pid);

    string sm_addr_str = "na+sm://"s + to_string(adafs_daemon_pid) + "/0";
    margo_addr_lookup(margo_ipc_id_, sm_addr_str.c_str(), &daemon_svr_addr_);

    register_client_ipcs(mid);

//    for (int i = 0; i < 10; ++i) {
//        printf("Running %d iteration\n", i);
//        send_minimal_ipc(minimal_id);
//    }

    return true;
}

bool init_rpc_client() {
    string protocol_port = RPC_PROTOCOL;

    LD_LOG_DEBUG0(debug_fd, "Initializing Margo RPC client ...\n");

    // Start Margo (this will also initialize Argobots and Mercury internally)
    auto mid = margo_init(protocol_port.c_str(), MARGO_CLIENT_MODE, 1, -1);

    if (mid == MARGO_INSTANCE_NULL) {
        LD_LOG_DEBUG0(debug_fd, "[ERR]: margo_init failed to initialize the Margo RPC client\n");
        return false;
    }
    LD_LOG_DEBUG0(debug_fd, "Success.\n");

    margo_rpc_id_ = mid;

    register_client_rpcs(mid);

//    for (int i = 0; i < 10000; ++i) {
//        printf("Running %d iteration\n", i);
//        send_minimal_ipc(minimal_id);
//    }

    return true;
}

margo_instance_id ld_margo_ipc_id() {
    return margo_ipc_id_;
}

margo_instance_id ld_margo_rpc_id() {
    return margo_rpc_id_;
}


hg_addr_t daemon_addr() {
    return daemon_svr_addr_;
}

/**
 * This function is only called in the preload constructor
 */
void init_environment() {
    // init margo client for IPC
    auto err = init_ipc_client();
    assert(err);
    err = ipc_send_get_fs_config(ipc_config_id); // get fs configurations the daemon was started with.
    assert(err);
    err = init_rpc_client();
    assert(err);
    is_env_initialized = true;
    LD_LOG_DEBUG0(debug_fd, "Environment initialized.\n");
}

void init_passthrough_() {
    libc = dlopen("libc.so.6", RTLD_LAZY);
    libc_open = dlsym(libc, "open");
//    libc_fopen = dlsym(libc, "fopen");
//    libc_fopen64 = dlsym(libc, "fopen64");

    libc_unlink = dlsym(libc, "unlink");

    libc_close = dlsym(libc, "close");
//    libc___close = dlsym(libc, "__close");

    libc_stat = dlsym(libc, "stat");
    libc_fstat = dlsym(libc, "fstat");
    libc___xstat = dlsym(libc, "__xstat");
    libc___xstat64 = dlsym(libc, "__xstat64");
    libc___fxstat = dlsym(libc, "__fxstat");
    libc___fxstat64 = dlsym(libc, "__fxstat64");
    libc___lxstat = dlsym(libc, "__lxstat");
    libc___lxstat64 = dlsym(libc, "__lxstat64");

    libc_access = dlsym(libc, "access");

    libc_puts = dlsym(libc, "puts");

    libc_write = dlsym(libc, "write");
    libc_pwrite = dlsym(libc, "pwrite");
    libc_read = dlsym(libc, "read");
    libc_pread = dlsym(libc, "pread");
    libc_pread64 = dlsym(libc, "pread64");

    libc_lseek = dlsym(libc, "lseek");

    libc_truncate = dlsym(libc, "truncate");
    libc_ftruncate = dlsym(libc, "ftruncate");

    libc_dup = dlsym(libc, "dup");
    libc_dup2 = dlsym(libc, "dup2");

    debug_fd = fopen(LOG_PRELOAD_PATH, "a+");
    fs_config = make_shared<struct FsConfig>();
    LD_LOG_DEBUG0(debug_fd, "Passthrough initialized.\n");
}

void init_passthrough_if_needed() {
    pthread_once(&init_lib_thread, init_passthrough_);
}

/**
 * Called initially when preload library is used with the LD_PRELOAD environment variable
 */
void init_preload() {
    init_passthrough_if_needed();
    init_environment();
    printf("[INFO] preload init successful.\n");
}

/**
 * Called last when preload library is used with the LD_PRELOAD environment variable
 */
void destroy_preload() {
//    margo_diag_dump(margo_ipc_id_, "-", 0);
    LD_LOG_DEBUG0(debug_fd, "Freeing Mercury daemon addr ...\n");
    HG_Addr_free(margo_get_class(margo_ipc_id_), daemon_svr_addr_);
    LD_LOG_DEBUG0(debug_fd, "Finalizing Margo IPC client ...\n");
    margo_finalize(margo_ipc_id_);

    LD_LOG_DEBUG0(debug_fd, "Freeing Mercury RPC addresses ...\n");
    // free all rpc addresses in LRU map and finalize margo rpc
    auto free_all_addr = [&](const KVCache::node_type& n) {
        HG_Addr_free(margo_get_class(ld_margo_rpc_id()), n.value);
    };
    rpc_address_cache_.cwalk(free_all_addr);
    LD_LOG_DEBUG0(debug_fd, "Finalizing Margo RPC client ...\n");
    margo_finalize(margo_rpc_id_);
    LD_LOG_DEBUG0(debug_fd, "Preload library shut down.\n");

    fclose(debug_fd);
}