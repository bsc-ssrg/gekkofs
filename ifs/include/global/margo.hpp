#ifndef GKFS_MARGO_HPP
#define GKFS_MARGO_HPP

#include <margo.h>
#include <string>

namespace gkfs { namespace margo {

class Margo {
    private:
        margo_instance_id mid_;

    public:
        Margo(const std::string& addr,
              bool server_mode,
              bool use_auto_sm = false,
              bool use_progress_thread = false,
              unsigned int rpc_thread_count = 0);

        ~Margo();

        margo_instance_id get_instance_id();
};

}} // end namespace gkfs::margo

#endif //GKFS_MARGO_HPP
