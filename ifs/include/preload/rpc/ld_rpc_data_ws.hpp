
#ifndef IFS_PRELOAD_C_DATA_WS_HPP
#define IFS_PRELOAD_C_DATA_WS_HPP


namespace rpc_send {


struct ChunkStat {
    unsigned long chunk_size;
    unsigned long chunk_total;
    unsigned long chunk_free;
};

ssize_t write(const fuid_t fuid, const void* buf, const bool append_flag,
                       const off64_t in_offset, const size_t write_size,
                       const int64_t updated_metadentry_size);

ssize_t read(const fuid_t fuid, void* buf, const off64_t offset, const size_t read_size);

int trunc_data(const fuid_t fuid, size_t current_size, size_t new_size);

ChunkStat chunk_stat();

}


#endif //IFS_PRELOAD_C_DATA_WS_HPP
