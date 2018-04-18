
#include <global/rpc/rpc_types.hpp>
#include <daemon/handler/rpc_defs.hpp>

#include <daemon/adafs_ops/metadentry.hpp>

#include <daemon/db/db_ops.hpp>

using namespace std;

static hg_return_t rpc_minimal(hg_handle_t handle) {
    rpc_minimal_in_t in{};
    rpc_minimal_out_t out{};
    // Get input
    auto ret = margo_get_input(handle, &in);
    assert(ret == HG_SUCCESS);

    ADAFS_DATA->spdlogger()->debug("Got simple RPC with input {}", in.input);

    // Create output and send it
    out.output = in.input * 2;
    ADAFS_DATA->spdlogger()->debug("Sending output {}", out.output);
    auto hret = margo_respond(handle, &out);
    assert(hret == HG_SUCCESS);

    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    ADAFS_DATA->spdlogger()->debug("Done with minimal rpc handler!");

    return HG_SUCCESS;
}

DEFINE_MARGO_RPC_HANDLER(rpc_minimal)

static hg_return_t rpc_srv_mk_node(hg_handle_t handle) {
    rpc_mk_node_in_t in{};
    rpc_err_out_t out{};

    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS)
        ADAFS_DATA->spdlogger()->error("{}() Failed to retrieve input from handle", __func__);
    assert(ret == HG_SUCCESS);
    ADAFS_DATA->spdlogger()->debug("{}() Got RPC (from local {}) with path {}", __func__,
                                   (margo_get_info(handle)->context_id == ADAFS_DATA->host_id()), in.path);
    // create metadentry
    out.err = create_metadentry(in.path, in.mode);

    ADAFS_DATA->spdlogger()->debug("{}() Sending output err {}", __func__, out.err);
    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to respond");
    }

    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_mk_node)

static hg_return_t rpc_srv_access(hg_handle_t handle) {
    rpc_access_in_t in{};
    rpc_err_out_t out{};

    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS)
        ADAFS_DATA->spdlogger()->error("{}() Failed to retrieve input from handle", __func__);
    assert(ret == HG_SUCCESS);
    ADAFS_DATA->spdlogger()->debug("{}() Got RPC (from local {}) with path {}", __func__,
                                   (margo_get_info(handle)->context_id == ADAFS_DATA->host_id()), in.path);
    // access metadentry
    out.err = check_access_mask(in.path, in.mask);

    ADAFS_DATA->spdlogger()->debug("{}() Sending output err {}", __func__, out.err);
    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to respond");
    }

    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_access)

static hg_return_t rpc_srv_stat(hg_handle_t handle) {
    rpc_path_only_in_t in{};
    rpc_stat_out_t out{};
    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS)
        ADAFS_DATA->spdlogger()->error("{}() Failed to retrieve input from handle", __func__);
    assert(ret == HG_SUCCESS);
    ADAFS_DATA->spdlogger()->debug("Got srv stat RPC for path {}", in.path);
    // get the metadata
    string val;
    auto err = db_get_metadentry(in.path, val);
    // TODO set return values proper with errorcodes the whole way
    if (err) {
        out.err = 0;
        out.db_val = val.c_str();
    } else {
        // if db_get_metadentry didn't return anything we set the errno to ENOENT
        out.err = ENOENT;
    }

    ADAFS_DATA->spdlogger()->debug("Sending output mode {}", out.db_val);
    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to respond");
    }

    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_stat)

static hg_return_t rpc_srv_rm_node(hg_handle_t handle) {
    rpc_rm_node_in_t in{};
    rpc_err_out_t out{};

    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS)
        ADAFS_DATA->spdlogger()->error("{}() Failed to retrieve input from handle", __func__);
    assert(ret == HG_SUCCESS);
    ADAFS_DATA->spdlogger()->debug("Got remove node RPC with path {}", in.path);

    // Remove metadentry if exists on the node but also remove all chunks for that path
    out.err = remove_node(in.path);

    ADAFS_DATA->spdlogger()->debug("Sending output {}", out.err);
    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to respond");
    }
    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_rm_node)


static hg_return_t rpc_srv_update_metadentry(hg_handle_t handle) {
    rpc_update_metadentry_in_t in{};
    rpc_err_out_t out{};


    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS)
        ADAFS_DATA->spdlogger()->error("{}() Failed to retrieve input from handle", __func__);
    assert(ret == HG_SUCCESS);
    ADAFS_DATA->spdlogger()->debug("Got update metadentry RPC with path {}", in.path);

    // do update
    Metadata md{};
    auto err = get_metadentry(in.path, md);
    if (err == 0) {
        out.err = 0;
        if (in.inode_no_flag == HG_TRUE)
            md.inode_no(in.inode_no);
        if (in.block_flag == HG_TRUE)
            md.blocks(in.blocks);
        if (in.uid_flag == HG_TRUE)
            md.uid(in.uid);
        if (in.gid_flag == HG_TRUE)
            md.gid(in.gid);
        if (in.nlink_flag == HG_TRUE)
            md.link_count(in.nlink);
        if (in.size_flag == HG_TRUE)
            md.size(in.size);
        if (in.atime_flag == HG_TRUE)
            md.atime(in.atime);
        if (in.mtime_flag == HG_TRUE)
            md.mtime(in.mtime);
        if (in.ctime_flag == HG_TRUE)
            md.ctime(in.ctime);
        out.err = update_metadentry(in.path, md);
    } else {
        out.err = 1;
    }
    ADAFS_DATA->spdlogger()->debug("Sending output {}", out.err);
    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to respond");
    }

    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_update_metadentry)

static hg_return_t rpc_srv_update_metadentry_size(hg_handle_t handle) {
    rpc_update_metadentry_size_in_t in{};
    rpc_update_metadentry_size_out_t out{};


    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS)
        ADAFS_DATA->spdlogger()->error("{}() Failed to retrieve input from handle", __func__);
    assert(ret == HG_SUCCESS);
    ADAFS_DATA->spdlogger()->debug("{}() Got update metadentry size RPC with path {}", __func__, in.path);

    // do update
    size_t read_size;
    auto err = update_metadentry_size(in.path, in.size, in.offset, (in.append == HG_TRUE), read_size);
    if (err == 0) {
        out.err = 0;
        out.ret_size = read_size;
    } else {
        out.err = err;
        out.ret_size = 0;
    }
    ADAFS_DATA->spdlogger()->debug("{}() Sending output {}", __func__, out.err);
    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to respond", __func__);
    }

    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_update_metadentry_size)

static hg_return_t rpc_srv_get_metadentry_size(hg_handle_t handle) {
    rpc_path_only_in_t in{};
    rpc_get_metadentry_size_out_t out{};


    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS)
        ADAFS_DATA->spdlogger()->error("{}() Failed to retrieve input from handle", __func__);
    assert(ret == HG_SUCCESS);
    ADAFS_DATA->spdlogger()->debug("Got update metadentry size RPC with path {}", in.path);

    // do update
    size_t ret_size;
    auto err = get_metadentry_size(in.path, ret_size);
    if (err == 0) {
        out.err = 0;
        out.ret_size = ret_size;
    } else {
        out.err = err;
        out.ret_size = 0;
    }
    ADAFS_DATA->spdlogger()->debug("Sending output {}", out.err);
    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to respond");
    }

    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_get_metadentry_size)