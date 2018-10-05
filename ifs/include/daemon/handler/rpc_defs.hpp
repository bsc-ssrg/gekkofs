
#ifndef LFS_RPC_DEFS_HPP
#define LFS_RPC_DEFS_HPP

extern "C" {
#include <mercury.h>
#include <margo.h>
}

/* visible API for RPC operations */

DECLARE_MARGO_RPC_HANDLER(rpc_minimal)

DECLARE_MARGO_RPC_HANDLER(ipc_srv_fs_config)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_mk_node)

#ifdef HAS_SYMLINKS
DECLARE_MARGO_RPC_HANDLER(rpc_srv_mk_symlink)
#endif

DECLARE_MARGO_RPC_HANDLER(rpc_srv_access)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_stat)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_decr_size)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_rm_node)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_update_metadentry)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_get_metadentry_size)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_update_metadentry_size)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_get_dirents)

// data
DECLARE_MARGO_RPC_HANDLER(rpc_srv_read_data)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_write_data)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_trunc_data)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_chunk_stat)

#endif //LFS_RPC_DEFS_HPP
