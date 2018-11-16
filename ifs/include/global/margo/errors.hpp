#ifndef GKFS_MARGO_ERRORS_HPP
#define GKFS_MARGO_ERRORS_HPP

#include <margo.h>
#include <string>
#include <stdexcept>


namespace gkfs { namespace margo {


class Error : public std::runtime_error {
    public:
        Error(hg_return_t hg_error, const std::string& msg);
};

inline void handle_hg_ret(hg_return_t hg_error, const std::string& msg) {
    if(hg_error != HG_SUCCESS) {
        throw Error(hg_error, msg);
    }
}


}} // end namespace gkfs::margo


#endif //GKFS_MARGO_ERRORS_HPP
