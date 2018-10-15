#include <global/configure.hpp>
#include <preload/margo_ipc.hpp>
#include <preload/preload_util.hpp>
#include <boost/token_functions.hpp>
#include <boost/tokenizer.hpp>
extern "C" {
#include <margo.h>
}

void send_minimal_ipc(const hg_id_t minimal_id) {

    hg_handle_t handle;
    rpc_minimal_in_t in{};
    rpc_minimal_out_t out{};

    printf("minimal RPC is running...\n");

    /* create handle */
    auto local_addr = get_local_addr();
    if (local_addr == HG_ADDR_NULL) {
        CTX->log()->error("{}() Unable to lookup local addr", __func__);
        return;
    }
    auto ret = margo_create(ld_margo_rpc_id, local_addr, rpc_config_id, &handle);
    if (ret != HG_SUCCESS) {
        margo_addr_free(ld_margo_rpc_id, local_addr);
        CTX->log()->error("{}() creating handle for failed", __func__);
        return;
    }

    /* Send rpc. Note that we are also transmitting the bulk handle in the
     * input struct.  It was set above.
     */
    in.input = 42;
    printf("About to send RPC\n");
    margo_forward(handle, &in);
    printf("Waiting for response\n");
    /* decode response */
    ret = margo_get_output(handle, &out);
    assert(ret == HG_SUCCESS);

    printf("Got response ret: %d\n", out.output);

    /* clean up resources consumed by this rpc */
    margo_addr_free(ld_margo_rpc_id, local_addr);
    margo_free_output(handle, &out);
    margo_destroy(handle);

    printf("minimal RPC is done.\n");
}

/**
 * Gets fs configuration information from the running daemon and transfers it to the memory of the library
 * @return
 */
bool ipc_send_get_fs_config() {
    hg_handle_t handle;
    ipc_config_in_t in{};
    ipc_config_out_t out{};
    // fill in
    in.dummy = 0; // XXX should be removed. havent checked yet how empty input with margo works
    auto local_addr = get_local_addr();
    if (local_addr == HG_ADDR_NULL) {
        CTX->log()->error("{}() Unable to lookup local addr", __func__);
        return false;
    }
    auto ret = margo_create(ld_margo_rpc_id, local_addr, rpc_config_id, &handle);
    if (ret != HG_SUCCESS) {
        margo_addr_free(ld_margo_rpc_id, local_addr);
        CTX->log()->error("{}() creating handle for failed", __func__);
        return false;
    }
    CTX->log()->debug("{}() About to send get config IPC to daemon", __func__);
    int send_ret = HG_FALSE;
    for (int i = 0; i < RPC_TRIES; ++i) {
        send_ret = margo_forward_timed(handle, &in, RPC_TIMEOUT);
        if (send_ret == HG_SUCCESS) {
            break;
        }
    }
    if (send_ret == HG_SUCCESS) {
        /* decode response */
        CTX->log()->debug("{}() Waiting for response", __func__);
        ret = margo_get_output(handle, &out);
        if (ret == HG_SUCCESS) {
            if (!CTX->mountdir().empty() && CTX->mountdir() != out.mountdir) {
                CTX->log()->warn(
                        "{}() fs_conf mountdir {} and received out.mountdir {} mismatch detected! Using received mountdir",
                        __func__, CTX->mountdir(), out.mountdir);
                CTX->mountdir(out.mountdir);
            }
            CTX->fs_conf()->rootdir = out.rootdir;
            CTX->fs_conf()->atime_state = out.atime_state;
            CTX->fs_conf()->mtime_state = out.mtime_state;
            CTX->fs_conf()->ctime_state = out.ctime_state;
            CTX->fs_conf()->uid_state = out.uid_state;
            CTX->fs_conf()->gid_state = out.gid_state;
            CTX->fs_conf()->link_cnt_state = out.link_cnt_state;
            CTX->fs_conf()->blocks_state = out.blocks_state;
            CTX->fs_conf()->uid = out.uid;
            CTX->fs_conf()->gid = out.gid;
            CTX->fs_conf()->host_id = out.host_id;
            CTX->fs_conf()->host_size = out.host_size;
            CTX->fs_conf()->rpc_port = to_string(RPC_PORT);

            // split comma separated host string and create a hosts map
            string hosts_raw = out.hosts_raw;
            std::map<uint64_t, std::string> hostmap;
            boost::char_separator<char> sep(",");
            boost::tokenizer<boost::char_separator<char>> tok(hosts_raw, sep);
            uint64_t i = 0;
            for (auto&& s : tok) {
                hostmap[i++] = s;
            }
            CTX->fs_conf()->hosts = hostmap;
            CTX->log()->debug("{}() Got response with mountdir {}", __func__, out.mountdir);
        } else {
            CTX->log()->error("{}() Retrieving fs configurations from daemon", __func__);
        }
        /* clean up resources consumed by this rpc */
        margo_addr_free(ld_margo_rpc_id, local_addr);
        margo_free_output(handle, &out);
    } else {
        CTX->log()->warn("{}() timed out", __func__);
    }

    margo_destroy(handle);
    return ret == HG_SUCCESS;
}