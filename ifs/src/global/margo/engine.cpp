#include "global/margo/engine.hpp"
#include <stdexcept>

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


}} // end namespace gkfs::margo
