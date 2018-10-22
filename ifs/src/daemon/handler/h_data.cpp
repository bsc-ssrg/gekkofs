
#include <global/rpc/rpc_types.hpp>
#include <daemon/handler/rpc_defs.hpp>
#include <global/rpc/rpc_utils.hpp>
#include "global/global_defs.hpp"
#include <global/rpc/distributor.hpp>
#include <global/chunk_calc_util.hpp>
#include <daemon/adafs_daemon.hpp>
#include <daemon/backend/data/chunk_storage.hpp>


using namespace std;

struct write_chunk_args {
    fuid_t fuid;
    const char* buf;
    rpc_chnk_id_t chnk_id;
    size_t size;
    off64_t off;
    ABT_eventual eventual;
};

/**
 * Used by an argobots threads. Argument args has the following fields:
 * const fuid_t fuid;
   const char* buf;
   const rpc_chnk_id_t* chnk_id;
   size_t size;
   off64_t off;
   ABT_eventual* eventual;
 * This function is driven by the IO pool. so there is a maximum allowed number of concurrent IO operations per daemon.
 * This function is called by tasklets, as this function cannot be allowed to block.
 * @return written_size<size_t> is put into eventual and returned that way
 */
void write_file_abt(void* _arg) {
    // Unpack args
    auto* arg = static_cast<struct write_chunk_args*>(_arg);

    try {
        ADAFS_DATA->storage()->write_chunk(arg->fuid, arg->chnk_id,
                arg->buf, arg->size, arg->off, arg->eventual);
    } catch (const std::exception& e){
        ADAFS_DATA->spdlogger()->error("{}() Error writing chunk {} of file {}", __func__, arg->chnk_id, arg->fuid);
        auto wrote = 0;
        ABT_eventual_set(arg->eventual, &wrote, sizeof(size_t));
    }

}

struct read_chunk_args {
    fuid_t fuid;
    char* buf;
    rpc_chnk_id_t chnk_id;
    size_t size;
    off64_t off;
    ABT_eventual eventual;
};

/**
 * Used by an argobots threads. Argument args has the following fields:
 * const fuid_t fuid;
   char* buf;
   const rpc_chnk_id_t* chnk_id;
   size_t size;
   off64_t off;
   ABT_eventual* eventual;
 * This function is driven by the IO pool. so there is a maximum allowed number of concurrent IO operations per daemon.
 * This function is called by tasklets, as this function cannot be allowed to block.
 * @return read_size<size_t> is put into eventual and returned that way
 */
void read_file_abt(void* _arg) {
    //unpack args
    auto* arg = static_cast<struct read_chunk_args*>(_arg);

    try {
        ADAFS_DATA->storage()->read_chunk(arg->fuid, arg->chnk_id,
                arg->buf, arg->size, arg->off, arg->eventual);
    } catch (const std::exception& e){
        ADAFS_DATA->spdlogger()->error("{}() Error reading chunk {} of file {}", __func__, arg->chnk_id, arg->fuid);
        size_t read = 0;
        ABT_eventual_set(arg->eventual, &read, sizeof(size_t));
    }
}

/**
 * Free Argobots tasks and eventual constructs in a given vector until max_idx.
 * Nothing is done for a vector if nullptr is given
 * @param abt_tasks
 * @param abt_eventuals
 * @param max_idx
 * @return
 */
void cancel_abt_io(vector<ABT_task>* abt_tasks, vector<ABT_eventual>* abt_eventuals, uint64_t max_idx) {
    if (abt_tasks != nullptr) {
        for (uint64_t i = 0; i < max_idx; i++) {
            ABT_task_cancel(abt_tasks->at(i));
            ABT_task_free(&abt_tasks->at(i));
        }
    }
    if (abt_eventuals != nullptr) {
        for (uint64_t i = 0; i < max_idx; i++) {
            ABT_eventual_reset(abt_eventuals->at(i));
            ABT_eventual_free(&abt_eventuals->at(i));
        }
    }
}


static hg_return_t rpc_srv_write_data(hg_handle_t handle) {
    /*
     * 1. Setup
     */
    rpc_write_data_in_t in{};
    rpc_data_out_t out{};
    hg_bulk_t bulk_handle = nullptr;
    // default out for error
    out.res = EIO;
    out.io_size = 0;
    // Getting some information from margo
    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Could not get RPC input data with err {}", __func__, ret);
        return rpc_cleanup_respond(&handle, &in, &out, &bulk_handle);
    }
    auto hgi = margo_get_info(handle);
    auto mid = margo_hg_info_get_instance(hgi);
    auto bulk_size = margo_bulk_get_size(in.bulk_handle);
    ADAFS_DATA->spdlogger()->debug("{}() Got write RPC (local {}) with fuid {} size {} offset {}", __func__,
                                   (margo_get_info(handle)->context_id == ADAFS_DATA->host_id()), in.fuid, bulk_size,
                                   in.offset);
    /*
     * 2. Set up buffers for pull bulk transfers
     */
    void* bulk_buf; // buffer for bulk transfer
    vector<char*> bulk_buf_ptrs(in.chunk_n); // buffer-chunk offsets
    // create bulk handle and allocated memory for buffer with buf_sizes information
    ret = margo_bulk_create(mid, 1, nullptr, &in.total_chunk_size, HG_BULK_READWRITE, &bulk_handle);
    if (ret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to create bulk handle", __func__);
        return rpc_cleanup_respond(&handle, &in, &out, static_cast<hg_bulk_t*>(nullptr));
    }
    // access the internally allocated memory buffer and put it into buf_ptrs
    uint32_t actual_count;
    ret = margo_bulk_access(bulk_handle, 0, in.total_chunk_size, HG_BULK_READWRITE, 1, &bulk_buf,
                            &in.total_chunk_size, &actual_count);
    if (ret != HG_SUCCESS || actual_count != 1) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to access allocated buffer from bulk handle", __func__);
        return rpc_cleanup_respond(&handle, &in, &out, &bulk_handle);
    }
    // chnk_ids used by this host
    vector<uint64_t> chnk_ids_host(in.chunk_n);
    // counter to track how many chunks have been assigned
    auto chnk_id_curr = static_cast<uint64_t>(0);
    // chnk sizes per chunk for this host
    vector<uint64_t> chnk_sizes(in.chunk_n);
    // how much size is left to assign chunks for writing
    auto chnk_size_left_host = in.total_chunk_size;
    // temporary traveling pointer
    auto chnk_ptr = static_cast<char*>(bulk_buf);
    /*
     * consider the following cases:
     * 1. Very first chunk has offset or not and is serviced by this node
     * 2. If offset, will still be only 1 chunk written (small IO): (offset + bulk_size <= CHUNKSIZE) ? bulk_size
     * 3. If no offset, will only be 1 chunk written (small IO): (bulk_size <= CHUNKSIZE) ? bulk_size
     * 4. Chunks between start and end chunk have size of the CHUNKSIZE
     * 5. Last chunk (if multiple chunks are written): Don't write CHUNKSIZE but chnk_size_left for this destination
     *    Last chunk can also happen if only one chunk is written. This is covered by 2 and 3.
     */
    // temporary variables
    auto transfer_size = (bulk_size <= CHUNKSIZE) ? bulk_size : CHUNKSIZE;
    uint64_t origin_offset;
    uint64_t local_offset;
    // task structures for async writing
    vector<ABT_task> abt_tasks(in.chunk_n);
    vector<ABT_eventual> task_eventuals(in.chunk_n);
    vector<unique_ptr<struct write_chunk_args>> task_args(in.chunk_n);
    /*
     * 3. Calculate chunk sizes that correspond to this host, transfer data, and start tasks to write to disk
     */
    // Start to look for a chunk that hashes to this host with the first chunk in the buffer
    for (auto chnk_id_file = in.chunk_start; chnk_id_file < in.chunk_end || chnk_id_curr < in.chunk_n; chnk_id_file++) {
        // Continue if chunk does not hash to this host
        if (ADAFS_DATA->distributor()->locate_data(in.fuid, chnk_id_file) != ADAFS_DATA->host_id())
            continue;
        chnk_ids_host[chnk_id_curr] = chnk_id_file; // save this id to host chunk list
        // offset case. Only relevant in the first iteration of the loop and if the chunk hashes to this host
        if (chnk_id_file == in.chunk_start && in.offset > 0) {
            // if only 1 destination and 1 chunk (small write) the transfer_size == bulk_size
            auto offset_transfer_size = (in.offset + bulk_size <= CHUNKSIZE) ? bulk_size : static_cast<size_t>(
                    CHUNKSIZE - in.offset);
            ret = margo_bulk_transfer(mid, HG_BULK_PULL, hgi->addr, in.bulk_handle, 0,
                                      bulk_handle, 0, offset_transfer_size);
            if (ret != HG_SUCCESS) {
                ADAFS_DATA->spdlogger()->error(
                        "{}() Failed to pull data from client for chunk {} (startchunk {}; endchunk {}", __func__,
                        chnk_id_file, in.chunk_start, in.chunk_end - 1);
                cancel_abt_io(&abt_tasks, &task_eventuals, chnk_id_curr);
                return rpc_cleanup_respond(&handle, &in, &out, &bulk_handle);
            }
            bulk_buf_ptrs[chnk_id_curr] = chnk_ptr;
            chnk_sizes[chnk_id_curr] = offset_transfer_size;
            chnk_ptr += offset_transfer_size;
            chnk_size_left_host -= offset_transfer_size;
        } else {
            local_offset = in.total_chunk_size - chnk_size_left_host;
            // origin offset of a chunk is dependent on a given offset in a write operation
            if (in.offset > 0)
                origin_offset = (CHUNKSIZE - in.offset) + ((chnk_id_file - in.chunk_start) - 1) * CHUNKSIZE;
            else
                origin_offset = (chnk_id_file - in.chunk_start) * CHUNKSIZE;
            // last chunk might have different transfer_size
            if (chnk_id_curr == in.chunk_n - 1)
                transfer_size = chnk_size_left_host;
            ADAFS_DATA->spdlogger()->trace(
                    "{}() BULK_TRANSFER hostid {} fuid {} chnkid {} total_Csize {} Csize_left {} origin offset {} local offset {} transfersize {}",
                    __func__, ADAFS_DATA->host_id(), in.fuid, chnk_id_file, in.total_chunk_size, chnk_size_left_host,
                    origin_offset, local_offset, transfer_size);
            // RDMA the data to here
            ret = margo_bulk_transfer(mid, HG_BULK_PULL, hgi->addr, in.bulk_handle, origin_offset,
                                      bulk_handle, local_offset, transfer_size);
            if (ret != HG_SUCCESS) {
                ADAFS_DATA->spdlogger()->error(
                        "{}() Failed to pull data from client. fuid {} chunk {} (startchunk {}; endchunk {})", __func__,
                        in.fuid, chnk_id_file, in.chunk_start, (in.chunk_end - 1));
                cancel_abt_io(&abt_tasks, &task_eventuals, chnk_id_curr);
                return rpc_cleanup_respond(&handle, &in, &out, &bulk_handle);
            }
            bulk_buf_ptrs[chnk_id_curr] = chnk_ptr;
            chnk_sizes[chnk_id_curr] = transfer_size;
            chnk_ptr += transfer_size;
            chnk_size_left_host -= transfer_size;
        }
        // Delegate chunk I/O operation to local FS to an I/O dedicated ABT pool
        // Starting tasklets for parallel I/O
        ABT_eventual_create(sizeof(size_t), &task_eventuals[chnk_id_curr]); // written file return value
        auto task_arg = make_unique<struct write_chunk_args>();
        task_arg->fuid = in.fuid;
        task_arg->buf = bulk_buf_ptrs[chnk_id_curr];
        task_arg->chnk_id = chnk_ids_host[chnk_id_curr];
        task_arg->size = chnk_sizes[chnk_id_curr];
        // only the first chunk gets the offset. the chunks are sorted on the client side
        task_arg->off = (chnk_id_file == in.chunk_start) ? in.offset : 0;
        task_arg->eventual = task_eventuals[chnk_id_curr];
        task_args[chnk_id_curr] = std::move(task_arg);
        auto abt_ret = ABT_task_create(RPC_DATA->io_pool(), write_file_abt, &(*task_args[chnk_id_curr]),
                                       &abt_tasks[chnk_id_curr]);
        if (abt_ret != ABT_SUCCESS) {
            ADAFS_DATA->spdlogger()->error("{}() task create failed", __func__);
            cancel_abt_io(&abt_tasks, &task_eventuals, chnk_id_curr + 1);
            return rpc_cleanup_respond(&handle, &in, &out, &bulk_handle);
        }
        // next chunk
        chnk_id_curr++;

    }
    // Sanity check that all chunks where detected in previous loop
    if (chnk_size_left_host != 0)
        ADAFS_DATA->spdlogger()->warn("{}() Not all chunks were detected!!! Size left {}", __func__,
                                      chnk_size_left_host);
    /*
     * 4. Read task results and accumulate in out.io_size
     */
    out.io_size = 0;
    for (chnk_id_curr = 0; chnk_id_curr < in.chunk_n; chnk_id_curr++) {
        size_t* task_written_size;
        // wait causes the calling ult to go into BLOCKED state, implicitly yielding to the pool scheduler
        ABT_eventual_wait(task_eventuals[chnk_id_curr], (void**) &task_written_size);
        if (task_written_size == nullptr || *task_written_size == 0) {
            ADAFS_DATA->spdlogger()->error("{}() Writing file task for chunk {} failed and did return anything.",
                                           __func__, chnk_id_curr);
            /*
             * XXX We have to talk about how chunk errors are handled? Should we try to write again?
             * In any case we just ignore this for now and return the out.io_size with as much has been written
             * After all, we can decide on the semantics.
             */
        } else {
            out.io_size += *task_written_size; // add task written size to output size
        }
        ABT_eventual_free(&task_eventuals[chnk_id_curr]);
    }
    // Sanity check to see if all data has been written
    if (in.total_chunk_size != out.io_size)
        ADAFS_DATA->spdlogger()->warn("{}() total chunk size {} and out.io_size {} mismatch!", __func__,
                                      in.total_chunk_size, out.io_size);
    /*
     * 5. Respond and cleanup
     */
    out.res = 0; // Set errorcode to succcess
    ADAFS_DATA->spdlogger()->debug("{}() Sending output response {}", __func__, out.res);
    ret = rpc_cleanup_respond(&handle, &in, &out, &bulk_handle);
    // free tasks after responding
    for (auto&& task : abt_tasks) {
        ABT_task_join(task);
        ABT_task_free(&task);
    }
    return ret;
}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_write_data)

static hg_return_t rpc_srv_read_data(hg_handle_t handle) {
    /*
     * 1. Setup
     */
    rpc_read_data_in_t in{};
    rpc_data_out_t out{};
    hg_bulk_t bulk_handle = nullptr;
    // Set default out for error
    out.res = EIO;
    out.io_size = 0;
    // Getting some information from margo
    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Could not get RPC input data with err {}", __func__, ret);
        return rpc_cleanup_respond(&handle, &in, &out, &bulk_handle);
    }
    auto hgi = margo_get_info(handle);
    auto mid = margo_hg_info_get_instance(hgi);
    auto bulk_size = margo_bulk_get_size(in.bulk_handle);
    ADAFS_DATA->spdlogger()->debug("{}() Got read RPC (local {}) with fuid {} size {} offset {}", __func__,
                                   (margo_get_info(handle)->context_id == ADAFS_DATA->host_id()), in.fuid, bulk_size,
                                   in.offset);

    /*
     * 2. Set up buffers for pull bulk transfers
     */
    void* bulk_buf; // buffer for bulk transfer
    vector<char*> bulk_buf_ptrs(in.chunk_n); // buffer-chunk offsets
    // create bulk handle and allocated memory for buffer with buf_sizes information
    ret = margo_bulk_create(mid, 1, nullptr, &in.total_chunk_size, HG_BULK_READWRITE, &bulk_handle);
    if (ret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to create bulk handle", __func__);
        return rpc_cleanup_respond(&handle, &in, &out, static_cast<hg_bulk_t*>(nullptr));
    }
    // access the internally allocated memory buffer and put it into buf_ptrs
    uint32_t actual_count;
    ret = margo_bulk_access(bulk_handle, 0, in.total_chunk_size, HG_BULK_READWRITE, 1, &bulk_buf,
                            &in.total_chunk_size, &actual_count);
    if (ret != HG_SUCCESS || actual_count != 1) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to access allocated buffer from bulk handle", __func__);
        return rpc_cleanup_respond(&handle, &in, &out, &bulk_handle);
    }
    // chnk_ids used by this host
    vector<uint64_t> chnk_ids_host(in.chunk_n);
    // counter to track how many chunks have been assigned
    auto chnk_id_curr = static_cast<uint64_t>(0);
    // chnk sizes per chunk for this host
    vector<uint64_t> chnk_sizes(in.chunk_n);
    // local and origin offsets for bulk operations
    vector<uint64_t> local_offsets(in.chunk_n);
    vector<uint64_t> origin_offsets(in.chunk_n);
    // how much size is left to assign chunks for reading
    auto chnk_size_left_host = in.total_chunk_size;
    // temporary traveling pointer
    auto chnk_ptr = static_cast<char*>(bulk_buf);
    // temporary variables
    auto transfer_size = (bulk_size <= CHUNKSIZE) ? bulk_size : CHUNKSIZE;
    // tasks structures
    vector<ABT_task> abt_tasks(in.chunk_n);
    vector<ABT_eventual> task_eventuals(in.chunk_n);
    vector<unique_ptr<struct read_chunk_args>> task_args(in.chunk_n);
    /*
     * 3. Calculate chunk sizes that correspond to this host and start tasks to read from disk
     */
    // Start to look for a chunk that hashes to this host with the first chunk in the buffer
    for (auto chnk_id_file = in.chunk_start; chnk_id_file < in.chunk_end || chnk_id_curr < in.chunk_n; chnk_id_file++) {
        // Continue if chunk does not hash to this host
        if (ADAFS_DATA->distributor()->locate_data(in.fuid, chnk_id_file) != ADAFS_DATA->host_id())
            continue;
        chnk_ids_host[chnk_id_curr] = chnk_id_file; // save this id to host chunk list
        // Only relevant in the first iteration of the loop and if the chunk hashes to this host
        if (chnk_id_file == in.chunk_start && in.offset > 0) {
            // if only 1 destination and 1 chunk (small read) the transfer_size == bulk_size
            auto offset_transfer_size = (in.offset + bulk_size <= CHUNKSIZE) ? bulk_size : static_cast<size_t>(
                    CHUNKSIZE - in.offset);
            // Setting later transfer offsets
            local_offsets[chnk_id_curr] = 0;
            origin_offsets[chnk_id_curr] = 0;
            bulk_buf_ptrs[chnk_id_curr] = chnk_ptr;
            chnk_sizes[chnk_id_curr] = offset_transfer_size;
            // util variables
            chnk_ptr += offset_transfer_size;
            chnk_size_left_host -= offset_transfer_size;
        } else {
            local_offsets[chnk_id_curr] = in.total_chunk_size - chnk_size_left_host;
            // origin offset of a chunk is dependent on a given offset in a write operation
            if (in.offset > 0)
                origin_offsets[chnk_id_curr] =
                        (CHUNKSIZE - in.offset) + ((chnk_id_file - in.chunk_start) - 1) * CHUNKSIZE;
            else
                origin_offsets[chnk_id_curr] = (chnk_id_file - in.chunk_start) * CHUNKSIZE;
            // last chunk might have different transfer_size
            if (chnk_id_curr == in.chunk_n - 1)
                transfer_size = chnk_size_left_host;
            bulk_buf_ptrs[chnk_id_curr] = chnk_ptr;
            chnk_sizes[chnk_id_curr] = transfer_size;
            // util variables
            chnk_ptr += transfer_size;
            chnk_size_left_host -= transfer_size;
        }
        // Delegate chunk I/O operation to local FS to an I/O dedicated ABT pool
        // Starting tasklets for parallel I/O
        ABT_eventual_create(sizeof(size_t), &task_eventuals[chnk_id_curr]); // written file return value
        auto task_arg = make_unique<read_chunk_args>();
        task_arg->fuid = in.fuid;
        task_arg->buf = bulk_buf_ptrs[chnk_id_curr];
        task_arg->chnk_id = chnk_ids_host[chnk_id_curr];
        task_arg->size = chnk_sizes[chnk_id_curr];
        // only the first chunk gets the offset. the chunks are sorted on the client side
        task_arg->off = (chnk_id_file == in.chunk_start) ? in.offset : 0;
        task_arg->eventual = task_eventuals[chnk_id_curr];
        task_args[chnk_id_curr] = std::move(task_arg);
        auto abt_ret = ABT_task_create(RPC_DATA->io_pool(), read_file_abt, &(*task_args[chnk_id_curr]),
                                       &abt_tasks[chnk_id_curr]);
        if (abt_ret != ABT_SUCCESS) {
            ADAFS_DATA->spdlogger()->error("{}() task create failed", __func__);
            cancel_abt_io(&abt_tasks, &task_eventuals, chnk_id_curr + 1);
            return rpc_cleanup_respond(&handle, &in, &out, &bulk_handle);
        }
        chnk_id_curr++;
    }
    // Sanity check that all chunks where detected in previous loop
    if (chnk_size_left_host != 0)
        ADAFS_DATA->spdlogger()->warn("{}() Not all chunks were detected!!! Size left {}", __func__,
                                      chnk_size_left_host);
    /*
     * 4. Read task results and accumulate in out.io_size
     */
    out.io_size = 0;
    for (chnk_id_curr = 0; chnk_id_curr < in.chunk_n; chnk_id_curr++) {
        size_t* task_read_size;
        // wait causes the calling ult to go into BLOCKED state, implicitly yielding to the pool scheduler
        ABT_eventual_wait(task_eventuals[chnk_id_curr], (void**) &task_read_size);

        assert(task_read_size != nullptr);

        if(*task_read_size == 0){
            ADAFS_DATA->spdlogger()->warn("{}() Read task for chunk {} returned 0 bytes", __func__, chnk_id_curr);
            continue;
        }

        ret = margo_bulk_transfer(mid, HG_BULK_PUSH, hgi->addr, in.bulk_handle, origin_offsets[chnk_id_curr],
                bulk_handle, local_offsets[chnk_id_curr], *task_read_size);
        if (ret != HG_SUCCESS) {
            ADAFS_DATA->spdlogger()->error(
                    "{}() Failed push chnkid {} on fuid {} to client. origin offset {} local offset {} chunk size {}",
                    __func__, chnk_id_curr, in.fuid, origin_offsets[chnk_id_curr], local_offsets[chnk_id_curr],
                    chnk_sizes[chnk_id_curr]);
            cancel_abt_io(&abt_tasks, &task_eventuals, in.chunk_n);
            return rpc_cleanup_respond(&handle, &in, &out, &bulk_handle);
        }
        out.io_size += *task_read_size; // add task read size to output size
    }

    ADAFS_DATA->spdlogger()->trace("{}() total chunk size read {}/{}", __func__, out.io_size, in.total_chunk_size);

    /*
     * 5. Respond and cleanup
     */
    out.res = 0; // Set errorcode to succcess
    ADAFS_DATA->spdlogger()->debug("{}() Sending output response {}", __func__, out.res);
    ret = rpc_cleanup_respond(&handle, &in, &out, &bulk_handle);
    // free tasks after responding
    cancel_abt_io(&abt_tasks, &task_eventuals, in.chunk_n);
    return ret;
}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_read_data)

static hg_return_t rpc_srv_trunc_data(hg_handle_t handle) {
    rpc_trunc_in_t in{};
    rpc_err_out_t out{};

    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Could not get RPC input data with err {}", __func__, ret);
        throw runtime_error("Failed to get RPC input data");
    }
    ADAFS_DATA->spdlogger()->debug("{}() fuid: '{}', length: {}", __func__, in.fuid, in.length);

    unsigned int chunk_start = chnk_id_for_offset(in.length, CHUNKSIZE);

    // If we trunc in the the middle of a chunk, do not delete that chunk
    auto left_pad = chnk_lpad(in.length, CHUNKSIZE);
    if(left_pad != 0) {
        ADAFS_DATA->storage()->truncate_chunk(in.fuid, chunk_start, left_pad);
        ++chunk_start;
    }

    ADAFS_DATA->storage()->trim_chunk_space(in.fuid, chunk_start);

    ADAFS_DATA->spdlogger()->debug("{}() Sending output {}", __func__, out.err);
    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to respond");
    }
    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_trunc_data)

static hg_return_t rpc_srv_chunk_stat(hg_handle_t handle) {
    ADAFS_DATA->spdlogger()->trace("{}()", __func__);

    rpc_chunk_stat_out_t out{};
    // Get input
    auto chk_stat = ADAFS_DATA->storage()->chunk_stat();
    // Create output and send it
    out.chunk_size = chk_stat.chunk_size;
    out.chunk_total = chk_stat.chunk_total;
    out.chunk_free = chk_stat.chunk_free;
    auto hret = margo_respond(handle, &out);
    assert(hret == HG_SUCCESS);

    // Destroy handle when finished
    margo_destroy(handle);
    return HG_SUCCESS;
}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_chunk_stat)


