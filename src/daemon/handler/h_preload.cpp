/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  SPDX-License-Identifier: MIT
*/


#include <daemon/main.hpp>
#include <daemon/handler/rpc_defs.hpp>
#include "global/rpc/rpc_types.hpp"


using namespace std;

static hg_return_t rpc_srv_fs_config(hg_handle_t handle) {
    rpc_config_in_t in{};
    rpc_config_out_t out{};

    auto ret = margo_get_input(handle, &in);
    assert(ret == HG_SUCCESS);
    ADAFS_DATA->spdlogger()->debug("{}() Got config RPC", __func__);

    // get fs config
    out.mountdir = ADAFS_DATA->mountdir().c_str();
    out.rootdir = ADAFS_DATA->rootdir().c_str();
    out.atime_state = static_cast<hg_bool_t>(ADAFS_DATA->atime_state());
    out.mtime_state = static_cast<hg_bool_t>(ADAFS_DATA->mtime_state());
    out.ctime_state = static_cast<hg_bool_t>(ADAFS_DATA->ctime_state());
    out.link_cnt_state = static_cast<hg_bool_t>(ADAFS_DATA->link_cnt_state());
    out.blocks_state = static_cast<hg_bool_t>(ADAFS_DATA->blocks_state());
    out.uid = getuid();
    out.gid = getgid();
    out.hosts_raw = static_cast<hg_const_string_t>(ADAFS_DATA->hosts_raw().c_str());
    out.host_id = static_cast<hg_uint64_t>(ADAFS_DATA->host_id());
    out.host_size = static_cast<hg_uint64_t>(ADAFS_DATA->host_size());
    out.rpc_port = ADAFS_DATA->rpc_port();
    out.hostname_suffix = ADAFS_DATA->hostname_suffix().c_str();
    out.lookup_file = ADAFS_DATA->lookup_file().c_str();
    ADAFS_DATA->spdlogger()->debug("{}() Sending output configs back to library", __func__);
    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to respond to client to serve file system configurations",
                                       __func__);
    }

    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_fs_config)