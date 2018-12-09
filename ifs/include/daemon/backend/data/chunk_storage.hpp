#ifndef IFS_CHUNK_STORAGE_HPP
#define IFS_CHUNK_STORAGE_HPP

#include "global/global_defs.hpp"

#include <abt.h>
#include <limits.h>
#include <string>
#include <memory>

/* Forward declarations */
namespace spdlog {
    class logger;
}


class ChunkStorage {
    private:
        static constexpr const char * LOGGER_NAME = "ChunkStorage";

        std::shared_ptr<spdlog::logger> log;

        std::string root_path;
        size_t chunksize;
        inline std::string absolute(const std::string& internal_path) const;
        static inline std::string get_chunks_dir(const fuid_t fuid);
        static inline std::string get_chunk_path(const fuid_t fuid, unsigned int chunk_id);
        void init_chunk_space(const fuid_t fuid) const;

    public:
        ChunkStorage(const std::string& path, const size_t chunksize);
        void write_chunk(const fuid_t fuid, unsigned int chunk_id,
                         const char * buff, size_t size, off64_t offset,
                         ABT_eventual& eventual) const;
        void read_chunk(const fuid_t fuid, unsigned int chunk_id,
                         char * buff, size_t size, off64_t offset,
                         ABT_eventual& eventual) const;
        void trim_chunk_space(const fuid_t fuid, unsigned int chunk_start,
                unsigned int chunk_end = UINT_MAX);
        void delete_chunk(const fuid_t fuid, unsigned int chunk_id);
        void truncate_chunk(const fuid_t fuid, unsigned int chunk_id, off_t length);
        void destroy_chunk_space(const fuid_t fuid) const;
};

#endif //IFS_CHUNK_STORAGE_HPP
