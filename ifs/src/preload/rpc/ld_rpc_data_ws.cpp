#include <global/configure.hpp>
#include <preload/preload_util.hpp>
#include <preload/rpc/ld_rpc_data_ws.hpp>
#include <global/rpc/rpc_utils.hpp>
#include <global/rpc/distributor.hpp>
#include <global/chunk_calc_util.hpp>

#include <unordered_set>

using namespace std;

// TODO If we decide to keep this functionality with one segment, the function can be merged mostly.
// Code is mostly redundant

/**
 * Sends an RPC request to a specific node to pull all chunks that belong to him
 */
ssize_t rpc_send_write(const string& path, const void* buf, const bool append_flag, const off64_t in_offset,
                       const size_t write_size, const int64_t updated_metadentry_size) {
    assert(write_size > 0);
    // Calculate chunkid boundaries and numbers so that daemons know in which interval to look for chunks
    off64_t offset = in_offset;
    if (append_flag)
        offset = updated_metadentry_size - write_size;

    auto chnk_start = chnk_id_for_offset(offset, CHUNKSIZE);
    auto chnk_end = chnk_id_for_offset((offset + write_size) - 1, CHUNKSIZE);

    // Collect all chunk ids within count that have the same destination so that those are send in one rpc bulk transfer
    map<uint64_t, vector<uint64_t>> target_chnks{};
    // contains the target ids, used to access the target_chnks map. First idx is chunk with potential offset
    vector<uint64_t> targets{};
    // targets for the first and last chunk as they need special treatment
    uint64_t chnk_start_target = 0;
    uint64_t chnk_end_target = 0;
    for (uint64_t chnk_id = chnk_start; chnk_id <= chnk_end; chnk_id++) {
        auto target = CTX->distributor()->locate_data(path, chnk_id);
        if (target_chnks.count(target) == 0) {
            target_chnks.insert(make_pair(target, vector<uint64_t>{chnk_id}));
            targets.push_back(target);
        } else
            target_chnks[target].push_back(chnk_id);
        // set first and last chnk targets
        if (chnk_id == chnk_start)
            chnk_start_target = target;
        if (chnk_id == chnk_end)
            chnk_end_target = target;
    }
    // some helper variables for async RPC
    auto target_n = targets.size();
    vector<hg_handle_t> rpc_handles(target_n);
    vector<margo_request> rpc_waiters(target_n);
    vector<rpc_write_data_in_t> rpc_in(target_n);
    // register local target buffer for bulk access for IPC and RPC margo instance
    auto bulk_buf = const_cast<void*>(buf);
    hg_bulk_t ipc_bulk_handle = nullptr;
    hg_bulk_t rpc_bulk_handle = nullptr;
    auto size = make_shared<size_t>(write_size);
    auto ret = margo_bulk_create(ld_margo_rpc_id, 1, &bulk_buf, size.get(), HG_BULK_READ_ONLY, &rpc_bulk_handle);
    if (ret != HG_SUCCESS) {
        CTX->log()->error("{}() Failed to create rpc bulk handle", __func__);
        errno = EBUSY;
        return -1;
    }
    ret = margo_bulk_create(ld_margo_ipc_id, 1, &bulk_buf, size.get(), HG_BULK_READ_ONLY, &ipc_bulk_handle);
    if (ret != HG_SUCCESS) {
        CTX->log()->error("{}() Failed to create rpc bulk handle", __func__);
        errno = EBUSY;
        return -1;
    }
    // Issue non-blocking RPC requests and wait for the result later
    for (uint64_t i = 0; i < target_n; i++) {
        auto target = targets[i];
        auto total_chunk_size = target_chnks[target].size() * CHUNKSIZE; // total chunk_size for target
        if (target == chnk_start_target) // receiver of first chunk must subtract the offset from first chunk
            total_chunk_size -= chnk_lpad(offset, CHUNKSIZE);
        if (target == chnk_end_target) // receiver of last chunk must subtract
            total_chunk_size -= chnk_rpad(offset + write_size, CHUNKSIZE);
        // Fill RPC input
        rpc_in[i].path = path.c_str();
        rpc_in[i].offset = chnk_lpad(offset, CHUNKSIZE);// first offset in targets is the chunk with a potential offset
        rpc_in[i].chunk_n = target_chnks[target].size(); // number of chunks handled by that destination
        rpc_in[i].chunk_start = chnk_start; // chunk start id of this write
        rpc_in[i].chunk_end = chnk_end; // chunk end id of this write
        rpc_in[i].total_chunk_size = total_chunk_size; // total size to write
        rpc_in[i].bulk_handle = (target == CTX->fs_conf()->host_id) ? ipc_bulk_handle : rpc_bulk_handle;
        margo_create_wrap(ipc_write_data_id, rpc_write_data_id, target, rpc_handles[i], false);
        // Send RPC
        ret = margo_iforward(rpc_handles[i], &rpc_in[i], &rpc_waiters[i]);
        if (ret != HG_SUCCESS) {
            CTX->log()->error("{}() Unable to send non-blocking rpc for path {} and recipient {}", __func__, path,
                             target);
            errno = EBUSY;
            for (uint64_t j = 0; j < i + 1; j++) {
                margo_destroy(rpc_handles[j]);
            }
            // free bulk handles for buffer
            margo_bulk_free(rpc_bulk_handle);
            margo_bulk_free(ipc_bulk_handle);
            return -1;
        }
    }

    // Wait for RPC responses and then get response and add it to out_size which is the written size
    // All potential outputs are served to free resources regardless of errors, although an errorcode is set.
    ssize_t out_size = 0;
    ssize_t err = 0;
    for (unsigned int i = 0; i < target_n; i++) {
        // XXX We might need a timeout here to not wait forever for an output that never comes?
        ret = margo_wait(rpc_waiters[i]);
        if (ret != HG_SUCCESS) {
            CTX->log()->error("{}() Unable to wait for margo_request handle for path {} recipient {}", __func__, path,
                             targets[i]);
            errno = EBUSY;
            err = -1;
        }
        // decode response
        rpc_data_out_t out{};
        ret = margo_get_output(rpc_handles[i], &out);
        if (ret != HG_SUCCESS) {
            CTX->log()->error("{}() Failed to get rpc output for path {} recipient {}", __func__, path, targets[i]);
            err = -1;
        }
        CTX->log()->debug("{}() Got response {}", __func__, out.res);
        if (out.res != 0)
            errno = out.res;
        out_size += static_cast<size_t>(out.io_size);
        margo_free_output(rpc_handles[i], &out);
        margo_destroy(rpc_handles[i]);
    }
    // free bulk handles for buffer
    margo_bulk_free(rpc_bulk_handle);
    margo_bulk_free(ipc_bulk_handle);
    return (err < 0) ? err : out_size;
}

/**
 * Sends an RPC request to a specific node to push all chunks that belong to him
 */
ssize_t rpc_send_read(const string& path, void* buf, const off64_t offset, const size_t read_size) {
    // Calculate chunkid boundaries and numbers so that daemons know in which interval to look for chunks
    auto chnk_start = chnk_id_for_offset(offset, CHUNKSIZE); // first chunk number
    auto chnk_end = chnk_id_for_offset((offset + read_size - 1), CHUNKSIZE);

    // Collect all chunk ids within count that have the same destination so that those are send in one rpc bulk transfer
    map<uint64_t, vector<uint64_t>> target_chnks{};
    // contains the recipient ids, used to access the target_chnks map. First idx is chunk with potential offset
    vector<uint64_t> targets{};
    // targets for the first and last chunk as they need special treatment
    uint64_t chnk_start_target = 0;
    uint64_t chnk_end_target = 0;
    for (uint64_t chnk_id = chnk_start; chnk_id <= chnk_end; chnk_id++) {
        auto target = CTX->distributor()->locate_data(path, chnk_id);
        if (target_chnks.count(target) == 0) {
            target_chnks.insert(make_pair(target, vector<uint64_t>{chnk_id}));
            targets.push_back(target);
        } else
            target_chnks[target].push_back(chnk_id);
        // set first and last chnk targets
        if (chnk_id == chnk_start)
            chnk_start_target = target;
        if (chnk_id == chnk_end)
            chnk_end_target = target;
    }
    // some helper variables for async RPC
    auto target_n = targets.size();
    vector<hg_handle_t> rpc_handles(target_n);
    vector<margo_request> rpc_waiters(target_n);
    vector<rpc_read_data_in_t> rpc_in(target_n);
    // register local target buffer for bulk access for IPC and RPC margo instance
    auto bulk_buf = buf;
    hg_bulk_t ipc_bulk_handle = nullptr;
    hg_bulk_t rpc_bulk_handle = nullptr;
    auto size = make_shared<size_t>(read_size);
    auto ret = margo_bulk_create(ld_margo_rpc_id, 1, &bulk_buf, size.get(), HG_BULK_WRITE_ONLY, &rpc_bulk_handle);
    if (ret != HG_SUCCESS) {
        CTX->log()->error("{}() Failed to create rpc bulk handle", __func__);
        errno = EBUSY;
        return -1;
    }
    ret = margo_bulk_create(ld_margo_ipc_id, 1, &bulk_buf, size.get(), HG_BULK_WRITE_ONLY, &ipc_bulk_handle);
    if (ret != HG_SUCCESS) {
        CTX->log()->error("{}() Failed to create rpc bulk handle", __func__);
        errno = EBUSY;
        return -1;
    }
    // Issue non-blocking RPC requests and wait for the result later
    for (unsigned int i = 0; i < target_n; i++) {
        auto target = targets[i];
        auto total_chunk_size = target_chnks[target].size() * CHUNKSIZE;
        if (target == chnk_start_target) // receiver of first chunk must subtract the offset from first chunk
            total_chunk_size -= chnk_lpad(offset, CHUNKSIZE);
        if (target == chnk_end_target) // receiver of last chunk must subtract
            total_chunk_size -= chnk_rpad(offset + read_size, CHUNKSIZE);

        // Fill RPC input
        rpc_in[i].path = path.c_str();
        rpc_in[i].offset = chnk_lpad(offset, CHUNKSIZE);// first offset in targets is the chunk with a potential offset
        rpc_in[i].chunk_n = target_chnks[target].size(); // number of chunks handled by that destination
        rpc_in[i].chunk_start = chnk_start; // chunk start id of this write
        rpc_in[i].chunk_end = chnk_end; // chunk end id of this write
        rpc_in[i].total_chunk_size = total_chunk_size; // total size to write
        rpc_in[i].bulk_handle = (target == CTX->fs_conf()->host_id) ? ipc_bulk_handle : rpc_bulk_handle;
        margo_create_wrap(ipc_read_data_id, rpc_read_data_id, target, rpc_handles[i], false);
        // Send RPC
        ret = margo_iforward(rpc_handles[i], &rpc_in[i], &rpc_waiters[i]);
        if (ret != HG_SUCCESS) {
            CTX->log()->error("{}() Unable to send non-blocking rpc for path {} and recipient {}", __func__, path,
                             target);
            errno = EBUSY;
            for (uint64_t j = 0; j < i + 1; j++) {
                margo_destroy(rpc_handles[j]);
            }
            // free bulk handles for buffer
            margo_bulk_free(rpc_bulk_handle);
            margo_bulk_free(ipc_bulk_handle);
            return -1;
        }
    }

    // Wait for RPC responses and then get response and add it to out_size which is the read size
    // All potential outputs are served to free resources regardless of errors, although an errorcode is set.
    ssize_t out_size = 0;
    ssize_t err = 0;
    for (unsigned int i = 0; i < target_n; i++) {
        // XXX We might need a timeout here to not wait forever for an output that never comes?
        ret = margo_wait(rpc_waiters[i]);
        if (ret != HG_SUCCESS) {
            CTX->log()->error("{}() Unable to wait for margo_request handle for path {} recipient {}", __func__, path,
                             targets[i]);
            errno = EBUSY;
            err = -1;
        }
        // decode response
        rpc_data_out_t out{};
        ret = margo_get_output(rpc_handles[i], &out);
        if (ret != HG_SUCCESS) {
            CTX->log()->error("{}() Failed to get rpc output for path {} recipient {}", __func__, path, targets[i]);
            err = -1;
        }
        CTX->log()->debug("{}() Got response {}", __func__, out.res);
        if (out.res != 0)
            errno = out.res;
        out_size += static_cast<size_t>(out.io_size);
        margo_free_output(rpc_handles[i], &out);
        margo_destroy(rpc_handles[i]);
    }
    // free bulk handles for buffer
    margo_bulk_free(rpc_bulk_handle);
    margo_bulk_free(ipc_bulk_handle);
    return (err < 0) ? err : out_size;
}

int rpc_send_trunc_data(const std::string& path, size_t current_size, size_t new_size) {
    assert(current_size > new_size);
    hg_return_t ret;
    rpc_trunc_in_t in;
    in.path = path.c_str();
    in.length = new_size;

    bool error = false;

    // Find out which data server needs to delete chunks in order to contact only them
    const unsigned int chunk_start = chnk_id_for_offset(new_size, CHUNKSIZE);
    const unsigned int chunk_end = chnk_id_for_offset(current_size - new_size - 1, CHUNKSIZE);
    std::unordered_set<unsigned int> hosts;
    for(unsigned int chunk_id = chunk_start; chunk_id <= chunk_end; ++chunk_id) {
        hosts.insert(CTX->distributor()->locate_data(path, chunk_id));
    }

    std::vector<hg_handle_t> rpc_handles(hosts.size());
    std::vector<margo_request> rpc_waiters(hosts.size());
    unsigned int req_num = 0;
    for (const auto& host: hosts) {
        ret = margo_create_wrap(ipc_trunc_data_id, rpc_trunc_data_id, host, rpc_handles[req_num], false);
        if (ret != HG_SUCCESS) {
            CTX->log()->error("{}() Unable to create Mercury handle for host: ", __func__, host);
            break;
        }

        // send async rpc
        ret = margo_iforward(rpc_handles[req_num], &in, &rpc_waiters[req_num]);
        if (ret != HG_SUCCESS) {
            CTX->log()->error("{}() Failed to send request to host: {}", __func__, host);
            break;
        }
        ++req_num;
    }

    if(req_num < hosts.size()) {
        // An error occurred. Cleanup and return
        CTX->log()->error("{}() Error -> sent only some requests {}/{}. Cancelling request...", __func__, req_num, hosts.size());
        for(unsigned int i = 0; i < req_num; ++i) {
            margo_destroy(rpc_handles[i]);
        }
        errno = EIO;
        return -1;
    }

    assert(req_num == hosts.size());
    // Wait for RPC responses and then get response
    rpc_err_out_t out{};
    for (unsigned int i = 0; i < hosts.size(); ++i) {
        ret = margo_wait(rpc_waiters[i]);
        if (ret == HG_SUCCESS) {
            ret = margo_get_output(rpc_handles[i], &out);
            if (ret == HG_SUCCESS) {
                if(out.err){
                    CTX->log()->error("{}() received error response: {}", __func__, out.err);
                    error = true;
                }
            } else {
                // Get output failed
                CTX->log()->error("{}() while getting rpc output", __func__);
                error = true;
            }
        } else {
            // Wait failed
            CTX->log()->error("{}() Failed while waiting for response", __func__);
            error = true;
        }

        /* clean up resources consumed by this rpc */
        margo_free_output(rpc_handles[i], &out);
        margo_destroy(rpc_handles[i]);
    }

    if(error) {
        errno = EIO;
        return -1;
    }
    return 0;
}

ChunkStat rpc_get_chunk_stat() {
    CTX->log()->trace("{}()", __func__);
    rpc_minimal_in_t in;

    auto const host_size =  CTX->fs_conf()->host_size;
    std::vector<hg_handle_t> rpc_handles(host_size);
    std::vector<margo_request> rpc_waiters(host_size);

    hg_return_t hg_ret;

    for (unsigned int target_host = 0; target_host < host_size; ++target_host) {
        //Setup rpc input parameters for each host
        hg_ret = margo_create_wrap(ipc_chunk_stat_id, rpc_chunk_stat_id,
                                   target_host, rpc_handles[target_host], false);
        if (hg_ret != HG_SUCCESS) {
            std::runtime_error("Failed to create margo handle");
        }
        // Send RPC
        CTX->log()->trace("{}() Sending RPC to host: {}", __func__, target_host);
        hg_ret = margo_iforward(rpc_handles[target_host],
                                &in,
                                &rpc_waiters[target_host]);
        if (hg_ret != HG_SUCCESS) {
            CTX->log()->error("{}() Unable to send non-blocking chunk_stat to recipient {}", __func__, target_host);
            for (unsigned int i = 0; i <= target_host; i++) {
                margo_destroy(rpc_handles[i]);
            }
            throw std::runtime_error("Failed to forward non-blocking rpc request");
        }
    }
    unsigned long chunk_size = CHUNKSIZE;
    unsigned long chunk_total = 0;
    unsigned long chunk_free = 0;

    for (unsigned int target_host = 0; target_host < host_size; ++target_host) {
        hg_ret = margo_wait(rpc_waiters[target_host]);
        if (hg_ret != HG_SUCCESS) {
            throw std::runtime_error(fmt::format("Failed while waiting for rpc completion. target host: {}", target_host));
        }
        rpc_chunk_stat_out_t out{};
        hg_ret = margo_get_output(rpc_handles[target_host], &out);
        if (hg_ret != HG_SUCCESS) {
            throw std::runtime_error(fmt::format("Failed to get rpc output for target host: {}", target_host));
        }

        assert(out.chunk_size == chunk_size);
        chunk_total += out.chunk_total;
        chunk_free  += out.chunk_free;

        margo_free_output(rpc_handles[target_host], &out);
        margo_destroy(rpc_handles[target_host]);
    }

    return {chunk_size, chunk_total, chunk_free};
}
