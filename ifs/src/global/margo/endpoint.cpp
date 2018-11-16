#include "global/margo/engine.hpp"
#include "global/margo/errors.hpp"

#include <stdexcept>
#include <random>
#include <cassert>
#include <thread>


namespace gkfs { namespace margo {


Endpoint::Endpoint()
    : engine_(nullptr)
    , addr_(HG_ADDR_NULL)
{}

Endpoint::Endpoint(const Engine& engine, hg_addr_t addr)
    : engine_(&engine)
    , addr_(addr)
{}

/* Copy constructor */
Endpoint::Endpoint(const Endpoint& other)
    : engine_(other.engine_)
    , addr_(HG_ADDR_NULL)
{
    if (other.addr_ != HG_ADDR_NULL) {
        auto ret = margo_addr_dup(engine_->mid_, other.addr_, &addr_);
        if (ret != HG_SUCCESS) {
            throw Error(ret, "Failed to duplicate address");
        }
    }
}

/* Move constructor */
Endpoint::Endpoint(Endpoint&& other)
    : engine_(other.engine_)
    , addr_(other.addr_)
{
    other.addr_ = HG_ADDR_NULL;
}

/* Copy-assignment operator */
Endpoint& Endpoint::operator=(const Endpoint& other) {
    if (&other == this) {
        return *this;
    }
    if (addr_ != HG_ADDR_NULL) {
        auto ret = margo_addr_free(engine_->mid_, addr_);
        if (ret != HG_SUCCESS) {
            throw Error(ret, "Failed to free address");
        }
    }
    engine_ = other.engine_;
    if (other.addr_ != HG_ADDR_NULL) {
        auto ret = margo_addr_dup(engine_->mid_, other.addr_, &addr_);
        if (ret != HG_SUCCESS) {
            throw Error(ret, "Failed to duplicate address");
        }
    } else {
        addr_ = HG_ADDR_NULL;
    }
    return *this;
}

/* Move-assignment operator */
Endpoint& Endpoint::operator=(Endpoint&& other) {
    if (&other == this) {
        return *this;
    }
    if (addr_ != HG_ADDR_NULL) {
        auto ret = margo_addr_free(engine_->mid_, addr_);
        if (ret != HG_SUCCESS) {
            throw Error(ret, "Failed to free address");
        }
    }
    engine_ = other.engine_;
    addr_ = other.addr_;
    other.addr_ = HG_ADDR_NULL;
    return *this;
}

Endpoint::~Endpoint() {
    if (addr_ != HG_ADDR_NULL) {
        auto ret = margo_addr_free(engine_->mid_, addr_);
        assert(ret == HG_SUCCESS);
    }
}

Endpoint::operator std::string() const {
    if(addr_ == HG_ADDR_NULL) {
        return "NULL_ADDRESS";
    }

    hg_size_t size;
    auto ret = margo_addr_to_string(engine_->mid_, NULL, &size, addr_);
    if (ret != HG_SUCCESS) {
        throw Error(ret, "Failed to convert address to string");
    }

    std::vector<char> result(size);

    ret = margo_addr_to_string(engine_->mid_, result.data(), &size, addr_);
    if (ret != HG_SUCCESS) {
        throw Error(ret, "Failed to convert address to string");
    }
    return std::string(result.data());
}


}} // end namespace gkfs::margo
