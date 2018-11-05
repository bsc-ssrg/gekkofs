#include <global/configure.hpp>
#include <preload/rpc/ld_rpc_metadentry.hpp>
#include <global/rpc/rpc_utils.hpp>
#include <global/rpc/distributor.hpp>
#include <global/rpc/rpc_types.hpp>
#include <preload/open_dir.hpp>
#include <daemon/adafs_daemon.hpp>

using namespace std;
using ns = chrono::nanoseconds;
using get_time = chrono::steady_clock;

inline hg_return_t margo_forward_timed_wrap_timer(hg_handle_t& handle, void* in_struct, const char* func) {
    auto start_t = get_time::now();
    auto ret = margo_forward_timed_wrap(handle, in_struct);
    auto diff_count = chrono::duration_cast<ns>(get_time::now() - start_t).count();
    if (((diff_count) / 1000000.) > MARGO_FORWARD_TIMER_THRESHOLD)
        CTX->log()->info("{}() rpc_time: {} ms", func, ((diff_count) / 1000000.));
    return ret;
}

inline hg_return_t margo_forward_timed_wrap(hg_handle_t& handle, void* in_struct) {
    hg_return_t ret = HG_OTHER_ERROR;

    for (int i = 0; i < RPC_TRIES; ++i) {
        ret = margo_forward_timed(handle, in_struct, RPC_TIMEOUT);
        if (ret == HG_SUCCESS) {
            break;
        }
    }
    return ret;
}

int rpc_send_mk_node(const std::string& path, const mode_t mode) {
    hg_handle_t handle;
    rpc_mk_node_in_t in{};
    rpc_err_out_t out{};
    int err = EUNKNOWN;
    // fill in
    in.path = path.c_str();
    in.mode = mode;
    // Create handle
    CTX->log()->debug("{}() Creating Mercury handle ...", __func__);
    auto ret = margo_create_wrap(rpc_mk_node_id, path, handle);
    if (ret != HG_SUCCESS) {
        errno = EBUSY;
        return -1;
    }
    // Send rpc
    CTX->log()->debug("{}() About to send RPC ...", __func__);
#if defined(MARGO_FORWARD_TIMER)
    ret = margo_forward_timed_wrap_timer(handle, &in, __func__);
#else
    ret = margo_forward_timed_wrap(handle, &in);
#endif
    // Get response
    if (ret == HG_SUCCESS) {
        CTX->log()->trace("{}() Waiting for response", __func__);
        ret = margo_get_output(handle, &out);
        if (ret == HG_SUCCESS) {
            CTX->log()->debug("{}() Got response success: {}", __func__, out.err);
            err = out.err;
        } else {
            // something is wrong
            errno = EBUSY;
            CTX->log()->error("{}() while getting rpc output", __func__);
        }
        /* clean up resources consumed by this rpc */
        margo_free_output(handle, &out);
    } else {
        CTX->log()->warn("{}() timed out");
        errno = EBUSY;
    }
    margo_destroy(handle);
    return err;
}

int rpc_send_access(const std::string& path, const int mask) {
    hg_handle_t handle;
    rpc_access_in_t in{};
    rpc_err_out_t out{};
    int err = 0;
    // fill in
    in.path = path.c_str();
    in.mask = mask;
    CTX->log()->debug("{}() Creating Mercury handle ...", __func__);
    auto ret = margo_create_wrap(rpc_access_id, path, handle);
    if (ret != HG_SUCCESS) {
        errno = EBUSY;
        return -1;
    }
    // Send rpc
    CTX->log()->debug("{}() About to send RPC ...", __func__);
#if defined(MARGO_FORWARD_TIMER)
    ret = margo_forward_timed_wrap_timer(handle, &in, __func__);
#else
    ret = margo_forward_timed_wrap(handle, &in);
#endif
    // Get response
    if (ret != HG_SUCCESS) {
        CTX->log()->error("{}() timed out");
        errno = EBUSY;
        margo_destroy(handle);
        return -1;
    }

    ret = margo_get_output(handle, &out);
    if (ret != HG_SUCCESS) {
        CTX->log()->error("{}() while getting rpc output", __func__);
        errno = EBUSY;
        margo_destroy(handle);
        return -1;
    }

    CTX->log()->debug("{}() Got response with error: {}", __func__, out.err);

    if(out.err != 0){
        //In case of error out.err contains the
        //corresponding value of errno
        errno = out.err;
        err = -1;
    }

    margo_free_output(handle, &out);
    margo_destroy(handle);
    return err;
}

int rpc_send_stat(const std::string& path, string& attr) {
    hg_handle_t handle;
    rpc_path_only_in_t in{};
    rpc_stat_out_t out{};
    int err = 0;
    // fill in
    in.path = path.c_str();
    CTX->log()->debug("{}() Creating Mercury handle ...", __func__);
    auto ret = margo_create_wrap(rpc_stat_id, path, handle);
    if (ret != HG_SUCCESS) {
        errno = EBUSY;
        return -1;
    }
    // Send rpc
#if defined(MARGO_FORWARD_TIMER)
    ret = margo_forward_timed_wrap_timer(handle, &in, __func__);
#else
    ret = margo_forward_timed_wrap(handle, &in);
#endif
    // Get response
    if (ret != HG_SUCCESS) {
        errno = EBUSY;
        CTX->log()->error("{}() timed out");
        margo_destroy(handle);
        return -1;
    }

    ret = margo_get_output(handle, &out);
    if (ret != HG_SUCCESS) {
        CTX->log()->error("{}() while getting rpc output", __func__);
        errno = EBUSY;
        margo_free_output(handle, &out);
        margo_destroy(handle);
        return -1;
    }

    CTX->log()->debug("{}() Got response success: {}", __func__, out.err);

    if(out.err != 0) {
        err = -1;
        errno = out.err;
    } else {
        attr = out.db_val;
    }

    /* clean up resources consumed by this rpc */
    margo_free_output(handle, &out);
    margo_destroy(handle);
    return err;
}

int rpc_send_decr_size(const std::string& path, size_t length) {
    hg_handle_t handle;
    rpc_trunc_in_t in{};
    int err = 0;
    in.path = path.c_str();
    in.length = length;

    CTX->log()->debug("{}() Creating Mercury handle ...", __func__);
    auto ret = margo_create_wrap(rpc_decr_size_id, path, handle);
    if (ret != HG_SUCCESS) {
        errno = EBUSY;
        return -1;
    }

    // Send rpc
#if defined(MARGO_FORWARD_TIMER)
    ret = margo_forward_timed_wrap_timer(handle, &in, __func__);
#else
    ret = margo_forward_timed_wrap(handle, &in);
#endif
    // Get response
    if (ret != HG_SUCCESS) {
        CTX->log()->error("{}() timed out", __func__);
        margo_destroy(handle);
        errno = EBUSY;
        return -1;
    }

    rpc_err_out_t out{};
    ret = margo_get_output(handle, &out);
    if (ret != HG_SUCCESS) {
        CTX->log()->error("{}() while getting rpc output", __func__);
        margo_free_output(handle, &out);
        margo_destroy(handle);
        errno = EBUSY;
        return -1;
    }

    CTX->log()->debug("{}() Got response: {}", __func__, out.err);

    if(out.err != 0){
        //In case of error out.err contains the
        //corresponding value of errno
        errno = out.err;
        err = -1;
    }

    margo_free_output(handle, &out);
    margo_destroy(handle);
    return err;
}

int rpc_send_rm_node(const std::string& path, const bool remove_metadentry_only) {
    hg_return_t ret;
    int err = 0; // assume we succeed
    // if metadentry should only removed only, send only 1 rpc to remove the metadata
    // else send an rpc to all hosts and thus broadcast chunk_removal.
    auto rpc_target_size = remove_metadentry_only ? static_cast<uint64_t>(1) : CTX->fs_conf()->host_size;

    CTX->log()->debug("{}() Creating Mercury handles for all nodes ...", __func__);
    vector<hg_handle_t> rpc_handles(rpc_target_size);
    vector<margo_request> rpc_waiters(rpc_target_size);
    vector<rpc_rm_node_in_t> rpc_in(rpc_target_size);
    // Send rpc to all nodes as all of them can have chunks for this path
    for (size_t i = 0; i < rpc_target_size; i++) {
        // fill in
        rpc_in[i].path = path.c_str();
        // create handle
        // if only the metadentry needs to removed send one rpc to metadentry's responsible node
        if (remove_metadentry_only)
            ret = margo_create_wrap(rpc_rm_node_id, path, rpc_handles[i]);
        else
            ret = margo_create_wrap_helper(rpc_rm_node_id, i, rpc_handles[i]);
        if (ret != HG_SUCCESS) {
            CTX->log()->warn("{}() Unable to create Mercury handle", __func__);
            // We use continue here to remove at least some data
            // XXX In the future we can discuss RPC retrying. This should be a function to be used in general
            errno = EBUSY;
            err = -1;
        }
        // send async rpc
        ret = margo_iforward(rpc_handles[i], &rpc_in[i], &rpc_waiters[i]);
        if (ret != HG_SUCCESS) {
            CTX->log()->warn("{}() Unable to create Mercury handle", __func__);
            errno = EBUSY;
            err = -1;
        }
    }

    // Wait for RPC responses and then get response
    for (size_t i = 0; i < rpc_target_size; i++) {
        // XXX We might need a timeout here to not wait forever for an output that never comes?
        ret = margo_wait(rpc_waiters[i]);
        if (ret != HG_SUCCESS) {
            CTX->log()->warn("{}() Unable to wait for margo_request handle for path {} recipient {}", __func__, path, i);
            errno = EBUSY;
            err = -1;
        }
        rpc_err_out_t out{};
        ret = margo_get_output(rpc_handles[i], &out);
        if (ret == HG_SUCCESS) {
            CTX->log()->debug("{}() Got response success: {}", __func__, out.err);
            if (err != 0) {
                errno = out.err;
                err = -1;
            }
        } else {
            // something is wrong
            errno = EBUSY;
            err = -1;
            CTX->log()->error("{}() while getting rpc output", __func__);
        }
        /* clean up resources consumed by this rpc */
        margo_free_output(rpc_handles[i], &out);
        margo_destroy(rpc_handles[i]);
    }
    return err;
}


int rpc_send_update_metadentry(const string& path, const Metadata& md, const MetadentryUpdateFlags& md_flags) {
    hg_handle_t handle;
    rpc_update_metadentry_in_t in{};
    rpc_err_out_t out{};
    int err = EUNKNOWN;
    // fill in
    // add data
    in.path = path.c_str();
    in.size = md_flags.size ? md.size() : 0;
    in.nlink = md_flags.link_count ? md.link_count() : 0;
    in.gid = md_flags.gid ? md.gid() : 0;
    in.uid = md_flags.uid ? md.uid() : 0;
    in.blocks = md_flags.blocks ? md.blocks() : 0;
    in.atime = md_flags.atime ? md.atime() : 0;
    in.mtime = md_flags.mtime ? md.mtime() : 0;
    in.ctime = md_flags.ctime ? md.ctime() : 0;
    // add data flags
    in.size_flag = bool_to_merc_bool(md_flags.size);
    in.nlink_flag = bool_to_merc_bool(md_flags.link_count);
    in.gid_flag = bool_to_merc_bool(md_flags.gid);
    in.uid_flag = bool_to_merc_bool(md_flags.uid);
    in.block_flag = bool_to_merc_bool(md_flags.blocks);
    in.atime_flag = bool_to_merc_bool(md_flags.atime);
    in.mtime_flag = bool_to_merc_bool(md_flags.mtime);
    in.ctime_flag = bool_to_merc_bool(md_flags.ctime);

    CTX->log()->debug("{}() Creating Mercury handle ...", __func__);
    auto ret = margo_create_wrap(rpc_update_metadentry_id, path, handle);
    if (ret != HG_SUCCESS) {
        errno = EBUSY;
        return -1;
    }
    // Send rpc
#if defined(MARGO_FORWARD_TIMER)
    ret = margo_forward_timed_wrap_timer(handle, &in, __func__);
#else
    ret = margo_forward_timed_wrap(handle, &in);
#endif
    // Get response
    if (ret == HG_SUCCESS) {
        CTX->log()->trace("{}() Waiting for response", __func__);
        ret = margo_get_output(handle, &out);
        if (ret == HG_SUCCESS) {
            CTX->log()->debug("{}() Got response success: {}", __func__, out.err);
            err = out.err;
        } else {
            // something is wrong
            errno = EBUSY;
            CTX->log()->error("{}() while getting rpc output", __func__);
        }
        /* clean up resources consumed by this rpc */
        margo_free_output(handle, &out);
    } else {
        CTX->log()->warn("{}() timed out");
        errno = EBUSY;
    }

    margo_destroy(handle);
    return err;
}

int rpc_send_update_metadentry_size(const string& path, const size_t size, const off64_t offset, const bool append_flag,
                                    off64_t& ret_size) {
    hg_handle_t handle;
    rpc_update_metadentry_size_in_t in{};
    rpc_update_metadentry_size_out_t out{};
    // add data
    in.path = path.c_str();
    in.size = size;
    in.offset = offset;
    if (append_flag)
        in.append = HG_TRUE;
    else
        in.append = HG_FALSE;
    int err = EUNKNOWN;

    CTX->log()->debug("{}() Creating Mercury handle ...", __func__);
    auto ret = margo_create_wrap(rpc_update_metadentry_size_id, path, handle);
    if (ret != HG_SUCCESS) {
        errno = EBUSY;
        return -1;
    }
    // Send rpc
#if defined(MARGO_FORWARD_TIMER)
    ret = margo_forward_timed_wrap_timer(handle, &in, __func__);
#else
    ret = margo_forward_timed_wrap(handle, &in);
#endif
    // Get response
    if (ret == HG_SUCCESS) {
        CTX->log()->trace("{}() Waiting for response", __func__);
        ret = margo_get_output(handle, &out);
        if (ret == HG_SUCCESS) {
            CTX->log()->debug("{}() Got response success: {}", __func__, out.err);
            err = out.err;
            ret_size = out.ret_size;
        } else {
            // something is wrong
            errno = EBUSY;
            ret_size = 0;
            CTX->log()->error("{}() while getting rpc output", __func__);
        }
        /* clean up resources consumed by this rpc */
        margo_free_output(handle, &out);
    } else {
        CTX->log()->warn("{}() timed out");
        errno = EBUSY;
    }
    margo_destroy(handle);
    return err;
}

int rpc_send_get_metadentry_size(const std::string& path, off64_t& ret_size) {
    hg_handle_t handle;
    rpc_path_only_in_t in{};
    rpc_get_metadentry_size_out_t out{};
    // add data
    in.path = path.c_str();
    int err = EUNKNOWN;

    CTX->log()->debug("{}() Creating Mercury handle ...", __func__);
    auto ret = margo_create_wrap(rpc_get_metadentry_size_id, path, handle);
    if (ret != HG_SUCCESS) {
        errno = EBUSY;
        return -1;
    }
    // Send rpc
#if defined(MARGO_FORWARD_TIMER)
    ret = margo_forward_timed_wrap_timer(handle, &in, __func__);
#else
    ret = margo_forward_timed_wrap(handle, &in);
#endif
    // Get response
    if (ret == HG_SUCCESS) {
        CTX->log()->trace("{}() Waiting for response", __func__);
        ret = margo_get_output(handle, &out);
        if (ret == HG_SUCCESS) {
            CTX->log()->debug("{}() Got response success: {}", __func__, out.err);
            err = out.err;
            ret_size = out.ret_size;
        } else {
            // something is wrong
            errno = EBUSY;
            ret_size = 0;
            CTX->log()->error("{}() while getting rpc output", __func__);
        }
        /* clean up resources consumed by this rpc */
        margo_free_output(handle, &out);
    } else {
        CTX->log()->warn("{}() timed out");
        errno = EBUSY;
    }
    margo_destroy(handle);
    return err;
}

/**
 * Sends an RPC request to a specific node to push all chunks that belong to him
 */
void rpc_send_get_dirents(OpenDir& open_dir){
    CTX->log()->trace("{}() called", __func__);
    auto const root_dir = open_dir.path();
    auto const targets = CTX->distributor()->locate_directory_metadata(root_dir);
    auto const host_size = targets.size();
    std::vector<hg_handle_t> rpc_handles(host_size);
    std::vector<margo_request> rpc_waiters(host_size);
    std::vector<rpc_get_dirents_in_t> rpc_in(host_size);
    std::vector<char*> recv_buffers(host_size);

    /* preallocate receiving buffer. The actual size is not known yet.
     *
     * On C++14 make_unique function also zeroes the newly allocated buffer.
     * It turns out that this operation is increadibly slow for such a big
     * buffer. Moreover we don't need a zeroed buffer here.
     */
    auto recv_buff = std::unique_ptr<char[]>(new char[RPC_DIRENTS_BUFF_SIZE]);
    const unsigned long int per_host_buff_size = RPC_DIRENTS_BUFF_SIZE / host_size;

    hg_return_t hg_ret;

    for(const auto& target_host: targets){

        CTX->log()->trace("{}() target_host: {}", __func__, target_host);
        //Setup rpc input parameters for each host
        rpc_in[target_host].path = root_dir.c_str();
        recv_buffers[target_host] = recv_buff.get() + (target_host * per_host_buff_size);

        hg_ret = margo_bulk_create(
                    ld_margo_rpc_id, 1,
                    reinterpret_cast<void**>(&recv_buffers[target_host]),
                    &per_host_buff_size,
                    HG_BULK_WRITE_ONLY, &(rpc_in[target_host].bulk_handle));
        if(hg_ret != HG_SUCCESS){
            throw std::runtime_error("Failed to create margo bulk handle");
        }

        hg_ret = margo_create_wrap_helper(rpc_get_dirents_id, target_host, rpc_handles[target_host]);
        if (hg_ret != HG_SUCCESS) {
            std::runtime_error("Failed to create margo handle");
        }
        // Send RPC
        CTX->log()->trace("{}() Sending RPC to host: {}", __func__, target_host);
        hg_ret = margo_iforward(rpc_handles[target_host],
                             &rpc_in[target_host],
                             &rpc_waiters[target_host]);
        if (hg_ret != HG_SUCCESS) {
            CTX->log()->error("{}() Unable to send non-blocking get_dirents on {} to recipient {}", __func__, root_dir, target_host);
            for (uint64_t i = 0; i <= target_host; i++) {
                margo_bulk_free(rpc_in[i].bulk_handle);
                margo_destroy(rpc_handles[i]);
            }
            throw std::runtime_error("Failed to forward non-blocking rpc request");
        }
    }

    for(unsigned int target_host = 0; target_host < host_size; target_host++){
        hg_ret = margo_wait(rpc_waiters[target_host]);
        if (hg_ret != HG_SUCCESS) {
            throw std::runtime_error(fmt::format("Failed while waiting for rpc completion. [root dir: {}, target host: {}]", root_dir, target_host));
        }
        rpc_get_dirents_out_t out{};
        hg_ret = margo_get_output(rpc_handles[target_host], &out);
        if (hg_ret != HG_SUCCESS) {
            throw std::runtime_error(fmt::format("Failed to get rpc output.. [path: {}, target host: {}]", root_dir, target_host));
        }

        bool* bool_ptr = reinterpret_cast<bool*>(recv_buffers[target_host]);
        char* names_ptr = recv_buffers[target_host] + (out.dirents_size * sizeof(bool));

        for(unsigned int i = 0; i < out.dirents_size; i++){

            FileType ftype = (*bool_ptr)? FileType::directory : FileType::regular;
            bool_ptr++;

            //Check that we are not outside the recv_buff for this specific host
            assert((names_ptr - recv_buffers[target_host]) > 0);
            assert(static_cast<unsigned long int>(names_ptr - recv_buffers[target_host]) < per_host_buff_size);

            auto name = std::string(names_ptr);
            names_ptr += name.size() + 1;

            open_dir.add(name, ftype);
        }

        margo_free_output(rpc_handles[target_host], &out);
        margo_bulk_free(rpc_in[target_host].bulk_handle);
        margo_destroy(rpc_handles[target_host]);
    }
}
