
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
extern margo_instance_id ld_margo_rpc_id;
// RPC IDs
extern hg_id_t rpc_config_id;
extern hg_id_t rpc_mk_node_id;
extern hg_id_t rpc_insert_node_id;
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

// function definitions

int metadata_to_stat(const std::string& path, const Metadata& md, struct stat& attr);

int get_daemon_pid();

bool read_system_hostfile();

hg_addr_t get_local_addr();

bool lookup_all_hosts();

void cleanup_addresses();

bool get_addr_by_hostid(uint64_t hostid, hg_addr_t& svr_addr);

hg_return margo_create_wrap_helper(const hg_id_t rpc_id, uint64_t recipient,
                                   hg_handle_t& handle);

hg_return margo_create_wrap(const hg_id_t rpc_id, const std::string&,
                            hg_handle_t& handle);


#endif //IFS_PRELOAD_UTIL_HPP
