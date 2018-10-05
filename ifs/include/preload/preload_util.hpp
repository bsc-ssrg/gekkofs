
#ifndef IFS_PRELOAD_UTIL_HPP
#define IFS_PRELOAD_UTIL_HPP

#include <preload/preload.hpp>
#include <global/metadata.hpp>
// third party libs
#include <string>
#include <iostream>

extern "C" {
#include <margo.h>
}

struct MetadentryUpdateFlags {
    bool atime = false;
    bool mtime = false;
    bool ctime = false;
    bool uid = false;
    bool gid = false;
    bool mode = false;
    bool link_count = false;
    bool size = false;
    bool blocks = false;
    bool path = false;
};

// Margo instances
extern margo_instance_id ld_margo_ipc_id;
extern margo_instance_id ld_margo_rpc_id;
// local daemon address for margo ipc client
extern hg_addr_t daemon_svr_addr;
// IPC IDs
extern hg_id_t ipc_minimal_id;
extern hg_id_t ipc_config_id;
extern hg_id_t ipc_mk_node_id;
extern hg_id_t ipc_access_id;
extern hg_id_t ipc_stat_id;
extern hg_id_t ipc_rm_node_id;
extern hg_id_t ipc_decr_size_id;
extern hg_id_t ipc_update_metadentry_id;
extern hg_id_t ipc_get_metadentry_size_id;
extern hg_id_t ipc_update_metadentry_size_id;
extern hg_id_t ipc_write_data_id;
extern hg_id_t ipc_read_data_id;
extern hg_id_t ipc_trunc_data_id;
extern hg_id_t ipc_get_dirents_id;
extern hg_id_t ipc_chunk_stat_id;
// RPC IDs
extern hg_id_t rpc_minimal_id;
extern hg_id_t rpc_mk_node_id;
extern hg_id_t rpc_stat_id;
extern hg_id_t rpc_access_id;
extern hg_id_t rpc_rm_node_id;
extern hg_id_t rpc_decr_size_id;
extern hg_id_t rpc_update_metadentry_id;
extern hg_id_t rpc_get_metadentry_size_id;
extern hg_id_t rpc_update_metadentry_size_id;
extern hg_id_t rpc_write_data_id;
extern hg_id_t rpc_read_data_id;
extern hg_id_t rpc_trunc_data_id;
extern hg_id_t rpc_get_dirents_id;
extern hg_id_t rpc_chunk_stat_id;

#ifdef HAS_SYMLINKS
extern hg_id_t ipc_mk_symlink_id;
extern hg_id_t rpc_mk_symlink_id;
#endif

// function definitions

bool is_fs_path(const char* path);

int metadata_to_stat(const std::string& path, const Metadata& md, struct stat& attr);

int get_daemon_pid();

bool read_system_hostfile();

bool lookup_all_hosts();

void cleanup_addresses();

bool get_addr_by_hostid(uint64_t hostid, hg_addr_t& svr_addr);

bool is_local_op(size_t recipient);

hg_return margo_create_wrap(hg_id_t ipc_id, hg_id_t rpc_id, const std::string&, hg_handle_t& handle, bool force_rpc);

hg_return margo_create_wrap(const hg_id_t ipc_id, const hg_id_t rpc_id, const size_t& recipient, hg_handle_t& handle, bool force_rpc);

#endif //IFS_PRELOAD_UTIL_HPP
