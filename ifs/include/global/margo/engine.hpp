#ifndef GKFS_MARGO_ENGINE_HPP
#define GKFS_MARGO_ENGINE_HPP

#include "endpoint.hpp"

#include <margo.h>
#include <string>
#include <stdexcept>

namespace gkfs { namespace margo {


class Engine {

    friend class Endpoint;

    private:
        margo_instance_id mid_;

    public:
        Engine(const std::string& addr,
               bool server_mode,
               bool use_auto_sm = false,
               bool use_progress_thread = false,
               unsigned int rpc_thread_count = 0);

        /* Copy constructor */
        Engine(const Engine&) = delete;
        /* Move-constructor */
        Engine(Engine&&) = delete;
        /* Copy-assignment operator */
        Engine& operator=(const Engine&) = delete;
        /* Move-assignment operator */
        Engine& operator=(Engine&& other) = delete;

        ~Engine();

        margo_instance_id get_instance_id();

        Endpoint addr_lookup_retry(const std::string& uri,
                                   unsigned int num_retries = 3) const;

        hg_handle_t create(hg_id_t rpc_id,
                           const Endpoint& endpoint) const;
};


}} // end namespace gkfs::margo

#endif //GKFS_MARGO_ENGINE_HPP
