#ifndef GKFS_RPC_ENGINE_HPP
#define GKFS_RPC_ENGINE_HPP

#include "global/margo/engine.hpp"
#include <string>
#include <unordered_map>


class RPCEngine {
    private:
        std::unordered_map<uint64_t, gkfs::margo::Endpoint> endpoints_;
        gkfs::margo::Engine margo_;
        gkfs::margo::Endpoint local_endpoint_;

    public:
        RPCEngine(const std::string& na_plugin, const std::string& local_addr);
        gkfs::margo::Engine& margo();
        void insert_endpoint(uint64_t host_id, const std::string& addr_str);
        hg_handle_t create(hg_id_t rpc_id, uint64_t host_id);
        hg_handle_t create_local(hg_id_t rpc_id);

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
