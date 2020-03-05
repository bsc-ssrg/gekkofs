/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GEKKOFS_DAEMON_DATA_HPP
#define GEKKOFS_DAEMON_DATA_HPP

#include <daemon/daemon.hpp>
#include <global/global_defs.hpp>

#include <string>
#include <vector>

extern "C" {
#include <abt.h>
#include <margo.h>
}

namespace gkfs {
namespace data {

class ChunkOpException : public std::runtime_error {
public:
    explicit ChunkOpException(const std::string& s) : std::runtime_error(s) {};
};

class ChunkWriteOpException : public ChunkOpException {
public:
    explicit ChunkWriteOpException(const std::string& s) : ChunkOpException(s) {};
};

class ChunkReadOpException : public ChunkOpException {
public:
    explicit ChunkReadOpException(const std::string& s) : ChunkOpException(s) {};
};

class ChunkMetaOpException : public ChunkOpException {
public:
    explicit ChunkMetaOpException(const std::string& s) : ChunkOpException(s) {};
};

/**
 * Classes to encapsulate asynchronous chunk operations.
 * All operations on chunk files must go through the Argobots' task queues.
 * Otherwise operations may overtake operations in the queues.
 * This applies to write, read, and truncate which may modify the middle of a chunk, essentially a write operation.
 *
 * Note: This class is not thread-safe.
 *
 * In the future, this class may be used to provide failure tolerance for IO tasks
 *
 * Abstract base class without public constructor:
 */
class ChunkOperation {

protected:

    std::string path_;

    std::vector<ABT_task> abt_tasks_;
    std::vector<ABT_eventual> task_eventuals_;

    virtual void cancel_all_tasks();

    explicit ChunkOperation(std::string path);

    ChunkOperation(std::string path, size_t n);

    ~ChunkOperation();
};


class ChunkTruncateOperation : public ChunkOperation {

private:
    struct chunk_truncate_args {
        const std::string* path;
        size_t size;
        ABT_eventual eventual;
    };

    std::vector<struct chunk_truncate_args> task_args_;

    static void truncate_abt(void* _arg);

public:

    explicit ChunkTruncateOperation(std::string path);

    ChunkTruncateOperation(std::string path, size_t n);

    void cancel_all_tasks() override;

    void truncate(size_t idx, size_t size);

    int wait_for_tasks();
};

class ChunkWriteOperation : public ChunkOperation {
private:

    struct chunk_write_args {
        const std::string* path;
        const char* buf;
        gkfs::rpc::chnk_id_t chnk_id;
        size_t size;
        off64_t off;
        ABT_eventual eventual;
    };

    std::vector<struct chunk_write_args> task_args_;

    static void write_file_abt(void* _arg);

public:

    ChunkWriteOperation(std::string path, size_t n);

    void cancel_all_tasks() override;

    void write_async(size_t idx, uint64_t chunk_id, const char* bulk_buf_ptr, size_t size, off64_t offset);

    std::pair<int, size_t> wait_for_tasks();

};


class ChunkReadOperation : public ChunkOperation {

private:

    struct chunk_read_args {
        const std::string* path;
        char* buf;
        gkfs::rpc::chnk_id_t chnk_id;
        size_t size;
        off64_t off;
        ABT_eventual eventual;
    };

    std::vector<struct chunk_read_args> task_args_;

    static void read_file_abt(void* _arg);

public:

    struct bulk_args {
        margo_instance_id mid;
        hg_addr_t origin_addr;
        hg_bulk_t origin_bulk_handle;
        std::vector<size_t>* origin_offsets;
        hg_bulk_t local_bulk_handle;
        std::vector<size_t>* local_offsets;
        std::vector<uint64_t>* chunk_ids;
    };

    ChunkReadOperation(std::string path, size_t n);

    void cancel_all_tasks() override;

    void read_async(size_t idx, uint64_t chunk_id, char* bulk_buf_ptr, size_t size, off64_t offset);

    std::pair<int, size_t>
    wait_for_tasks_and_push_back(const bulk_args& args);

};

} // namespace data
} // namespace gkfs

#endif //GEKKOFS_DAEMON_DATA_HPP
