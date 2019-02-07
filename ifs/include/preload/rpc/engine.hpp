#ifndef GKFS_RPC_ENGINE_HPP
#define GKFS_RPC_ENGINE_HPP

#include "global/margo/engine.hpp"
#include <string>


class RPCEngine {
    private:
        gkfs::margo::Engine margo_;

    public:
        RPCEngine(const std::string& na_plugin);
        margo_instance_id mid();

        /* RPC handles */
        hg_id_t rpc_config_id;
        hg_id_t rpc_mk_node_id;
        hg_id_t rpc_stat_id;
        hg_id_t rpc_access_id;
        hg_id_t rpc_rm_node_id;
        hg_id_t rpc_decr_size_id;
        hg_id_t rpc_update_metadentry_id;
        hg_id_t rpc_get_metadentry_size_id;
        hg_id_t rpc_update_metadentry_size_id;
        hg_id_t rpc_write_data_id;
        hg_id_t rpc_read_data_id;
        hg_id_t rpc_trunc_data_id;
        hg_id_t rpc_get_dirents_id;
        hg_id_t rpc_chunk_stat_id;
};


#endif // GKFS_RPC_ENGINE_HPP
