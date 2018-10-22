
#ifndef IFS_PRELOAD_C_DATA_WS_HPP
#define IFS_PRELOAD_C_DATA_WS_HPP

extern "C" {
#include <abt.h>
#include <mercury_types.h>
#include <mercury_proc_string.h>
#include <margo.h>
}

#include <global/rpc/rpc_types.hpp>
#include <preload/preload.hpp>

#include <iostream>


struct ChunkStat {
    unsigned long chunk_size;
    unsigned long chunk_total;
    unsigned long chunk_free;
};


ssize_t rpc_send_write(const fuid_t fuid, const void* buf, const bool append_flag, const off64_t in_offset,
                       const size_t write_size, const int64_t updated_metadentry_size);

ssize_t rpc_send_read(const fuid_t fuid, void* buf, const off64_t offset, const size_t read_size);

int rpc_send_trunc_data(const fuid_t fuid, size_t current_size, size_t new_size);

ChunkStat rpc_get_chunk_stat();

#endif //IFS_PRELOAD_C_DATA_WS_HPP
