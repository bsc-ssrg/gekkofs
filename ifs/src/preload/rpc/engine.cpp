#include "preload/rpc/engine.hpp"
#include "global/rpc/rpc_types.hpp"
#include "global/global_defs.hpp"


RPCEngine::RPCEngine(const std::string& na_plugin,
                     const std::string& local_addr)
    : margo_(na_plugin,
           false,  // server_mode
           true,   // use_auto_sm
           false,  // use_progress_thread
           1)      // rpc_thread_count
    , local_endpoint_(margo_.addr_lookup_retry(local_addr))
{
    /* Register RPCs */
    rpc_config_id = MARGO_REGISTER(margo_.get_instance_id(),
         hg_tag::fs_config,
         rpc_config_in_t,
         rpc_config_out_t,
         NULL);

    rpc_mk_node_id = MARGO_REGISTER(margo_.get_instance_id(),
         hg_tag::create,
         rpc_mk_node_in_t,
         rpc_err_out_t,
         NULL);

    rpc_access_id = MARGO_REGISTER(margo_.get_instance_id(),
         hg_tag::access,
         rpc_access_in_t,
         rpc_err_out_t,
         NULL);

    rpc_stat_id = MARGO_REGISTER(margo_.get_instance_id(),
         hg_tag::stat,
         rpc_path_only_in_t,
         rpc_stat_out_t,
         NULL);

    rpc_rm_node_id = MARGO_REGISTER(margo_.get_instance_id(),
         hg_tag::remove,
         rpc_rm_node_in_t,
         rpc_err_out_t,
         NULL);

    rpc_decr_size_id = MARGO_REGISTER(margo_.get_instance_id(),
        hg_tag::decr_size,
        rpc_trunc_in_t,
        rpc_err_out_t,
        NULL);

    rpc_update_metadentry_id = MARGO_REGISTER(margo_.get_instance_id(),
        hg_tag::update_metadentry,
        rpc_update_metadentry_in_t,
        rpc_err_out_t,
        NULL);

    rpc_get_metadentry_size_id = MARGO_REGISTER(margo_.get_instance_id(),
        hg_tag::get_metadentry_size,
        rpc_path_only_in_t,
        rpc_get_metadentry_size_out_t,
        NULL);

    rpc_update_metadentry_size_id = MARGO_REGISTER(margo_.get_instance_id(),
        hg_tag::update_metadentry_size,
        rpc_update_metadentry_size_in_t,
        rpc_update_metadentry_size_out_t,
        NULL);

    rpc_write_data_id = MARGO_REGISTER(margo_.get_instance_id(),
        hg_tag::write_data,
        rpc_write_data_in_t,
        rpc_data_out_t,
        NULL);

    rpc_read_data_id = MARGO_REGISTER(margo_.get_instance_id(),
        hg_tag::read_data,
        rpc_read_data_in_t,
        rpc_data_out_t,
        NULL);

    rpc_trunc_data_id = MARGO_REGISTER(margo_.get_instance_id(),
         hg_tag::trunc_data,
         rpc_trunc_in_t,
         rpc_err_out_t,
         NULL);

    rpc_get_dirents_id = MARGO_REGISTER(margo_.get_instance_id(),
         hg_tag::get_dirents,
         rpc_get_dirents_in_t,
         rpc_get_dirents_out_t,
         NULL);

    rpc_chunk_stat_id = MARGO_REGISTER(margo_.get_instance_id(),
        hg_tag::chunk_stat,
        rpc_chunk_stat_in_t,
        rpc_chunk_stat_out_t,
        NULL);
}

gkfs::margo::Engine& RPCEngine::margo() {
    return margo_;
}

void RPCEngine::insert_endpoint(uint64_t host_id, const std::string& addr_str) {
    endpoints_.insert(
            std::make_pair(host_id, margo_.addr_lookup_retry(addr_str)));
}

hg_handle_t RPCEngine::create(hg_id_t rpc_id, uint64_t host_id) {
    return margo_.create(rpc_id, endpoints_.at(host_id));
}

hg_handle_t RPCEngine::create_local(hg_id_t rpc_id) {
    return margo_.create(rpc_id, local_endpoint_);
}

