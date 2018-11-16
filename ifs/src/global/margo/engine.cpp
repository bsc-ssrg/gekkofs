#include "global/margo/engine.hpp"
#include "global/margo/errors.hpp"

#include <stdexcept>
#include <random>
#include <cassert>
#include <thread>


namespace gkfs { namespace margo {


Engine::Engine(const std::string& addr,
               bool server_mode,
               bool use_auto_sm,
               bool use_progress_thread,
               unsigned int rpc_thread_count)
{

    struct hg_init_info hg_options = {};
    hg_options.auto_sm = (use_auto_sm? HG_TRUE : HG_FALSE);
    hg_options.stats = HG_FALSE;
    hg_options.na_class = nullptr;

    mid_ = margo_init_opt(addr.c_str(),
                          (server_mode? MARGO_SERVER_MODE : MARGO_CLIENT_MODE),
                          &hg_options,
                          (use_progress_thread? HG_TRUE : HG_FALSE),
                          rpc_thread_count);

    if (mid_ == MARGO_INSTANCE_NULL) {
        throw std::runtime_error("failed to initialize Margo");
    }
}

Engine::~Engine() {
    margo_finalize(mid_);
}

margo_instance_id Engine::get_instance_id() {
    return mid_;
}

Endpoint Engine::addr_lookup_retry(const std::string& uri,
                                  unsigned int max_attempts) const {
    hg_return_t ret;
    hg_addr_t addr = HG_ADDR_NULL;
    std::random_device rd; // obtain a random number from hardware
    unsigned int attempts = 0;
    do {
        ret = margo_addr_lookup(mid_, uri.c_str(), &addr);
        if (ret == HG_SUCCESS) {
            return Endpoint(*this, addr);
        }
        // Wait a random amount of time and try again
        std::mt19937 g(rd()); // seed the random generator
        std::uniform_int_distribution<> distr(50, 50 * (attempts + 2)); // define the range
        std::this_thread::sleep_for(std::chrono::milliseconds(distr(g)));
    } while (++attempts < max_attempts);

    throw std::runtime_error("Failed to lookup address: " + uri);
}

hg_handle_t Engine::create(const hg_id_t rpc_id, const Endpoint& endpoint) const {
    hg_handle_t handle;
    handle_hg_ret(
         margo_create(mid_, endpoint.addr_, rpc_id, &handle),
         "Failed to create RPC handle"
    );
    return handle;
}


}} // end namespace gkfs::margo
