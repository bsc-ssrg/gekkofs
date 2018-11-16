#include "preload/rpc/ld_rpc_management.hpp"
#include "preload/rpc/engine.hpp"
#include "global/rpc/rpc_types.hpp"
#include <preload/preload_util.hpp>
#include <boost/type_traits/is_pointer.hpp> // see https://github.com/boostorg/tokenizer/issues/9
#include <boost/token_functions.hpp>
#include <boost/tokenizer.hpp>


namespace rpc_send {

/**
 * Gets fs configuration information from the running daemon and transfers it to the memory of the library
 * @return
 */
bool get_fs_config() {
    rpc_config_in_t in{};
    rpc_config_out_t out{};
    // fill in
    in.dummy = 0; // XXX should be removed. havent checked yet how empty input with margo works
    auto handle = CTX->rpc()->create_local(CTX->rpc()->rpc_config_id);
    CTX->log()->debug("{}() About to send get config RPC to daemon", __func__);

    int send_ret = HG_FALSE;
    for (int i = 0; i < RPC_TRIES; ++i) {
        send_ret = margo_forward_timed(handle, &in, RPC_TIMEOUT);
        if (send_ret == HG_SUCCESS) {
            break;
        }
    }
    if (send_ret != HG_SUCCESS) {
        CTX->log()->warn("{}() timed out", __func__);
        margo_destroy(handle);
        return false;
    }

    /* decode response */
    CTX->log()->debug("{}() Waiting for response", __func__);
    auto ret = margo_get_output(handle, &out);
    if (ret != HG_SUCCESS) {
        CTX->log()->error("{}() Retrieving fs configurations from daemon", __func__);
        margo_destroy(handle);
        return false;
    }

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
    CTX->fs_conf()->rpc_port = std::to_string(RPC_PORT);

    // split comma separated host string and create a hosts map
    std::string hosts_raw = out.hosts_raw;
    std::map<uint64_t, std::string> hostmap;
    boost::char_separator<char> sep(",");
    boost::tokenizer<boost::char_separator<char>> tok(hosts_raw, sep);
    uint64_t i = 0;
    for (auto&& s : tok) {
        hostmap[i++] = s;
    }
    CTX->fs_conf()->hosts = hostmap;
    CTX->log()->debug("{}() Got response with mountdir {}", __func__, out.mountdir);

    /* clean up resources consumed by this rpc */
    margo_free_output(handle, &out);
    margo_destroy(handle);
    return true;
}


}