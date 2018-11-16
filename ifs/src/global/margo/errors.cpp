#include "global/margo/errors.hpp"

#include <cassert>

namespace gkfs { namespace margo {


Error::Error(hg_return_t hg_error, const std::string& msg) :
    std::runtime_error(msg + ":" + HG_Error_to_string(hg_error))
{
    assert(hg_error != HG_SUCCESS);
}


}} // end namespace gkfs::margo
