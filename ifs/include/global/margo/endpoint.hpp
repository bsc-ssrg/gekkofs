#ifndef GKFS_MARGO_ENDPOINT_HPP
#define GKFS_MARGO_ENDPOINT_HPP

#include <margo.h>
#include <string>

namespace gkfs { namespace margo {


/* Forward declarations */
class Engine;

class Endpoint {

    friend class Engine;

    private:
        const Engine* engine_;
        hg_addr_t addr_;

    public:
        Endpoint();
        Endpoint(const Engine& m, hg_addr_t addr);

        /* Copy constructor */
        Endpoint(const Endpoint& other);
        /* Move constructor */
        Endpoint(Endpoint&& other);
        /* Copy-assignment operator */
        Endpoint& operator=(const Endpoint& other);
        /* Move-assignment operator */
        Endpoint& operator=(Endpoint&& other);

        ~Endpoint();

        operator std::string() const;
};


}} // end namespace gkfs::margo

#endif //GKFS_MARGO_ENDPOINT_HPP
