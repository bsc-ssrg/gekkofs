/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <global/ofi_util.hpp>
#include <fmt/format.h>

#include <cstring>

extern "C" {
#include <rdma/fabric.h>
}

//TODO remove
#include <iostream>

using namespace std;

namespace gkfs {
namespace rpc {

class OFIException : public std::system_error {
public:
    OFIException(const int err_code, const std::string& s) : std::system_error(err_code,
                                                                               std::system_category(), s) {};
};

static constexpr auto provider = "psm2";

/**
 * Translates an uri with a hostname or IP into the native psm2 address
 * @param hostname
 * @return addr string
 * @throws OFIException
 */
string ofi_get_psm2_address(const string& hostname) {
    struct fi_info* hints;
    struct fi_info* fi;
    hints = fi_allocinfo();
    if (!hints) {
        auto err_str = fmt::format("{}() Failed to allocate hints struct for fi_getinfo call", __func__);
        throw OFIException(EBUSY, err_str);
    }
    // hints is passed to fi_getinfo as const struct, so the cast here is safe
    hints->fabric_attr->prov_name = const_cast<char*>(provider);
    hints->addr_format = FI_ADDR_STR;
    hints->ep_attr->protocol = FI_PROTO_PSMX2;

    auto err = fi_getinfo(FI_VERSION(FI_MAJOR_VERSION, FI_MINOR_VERSION), hostname.c_str(), nullptr, 0, hints, &fi);
    if (err) {
        auto err_str = fmt::format("{}() Failed to get host by name through fi_getinfo", __func__);
        throw OFIException(err, err_str);
    }
    if (!fi) {
        auto err_str = fmt::format("{}() Failed to get host by name through fi_getinfo: no results", __func__);
        throw OFIException(EBUSY, err_str);
    }

    // below is an error case
    if (fi->next) {
        while (fi->next) {
            cout << "Name: " << hostname << endl;
            cout << "Src: " << (char*) fi->src_addr << "\" [" << fi->dest_addrlen << "]" << endl;
            cout << "Dst: \"" << (char*) fi->dest_addr << "\" [" << fi->dest_addrlen << "]" << endl;
            cout << "Format: " << fi->addr_format << endl;
            cout << "Prov: " << fi->fabric_attr->prov_name << endl;
            fi = fi->next;
        }
        auto err_str = fmt::format("{}() fi_getinfo: multiple results found for specific hostname", __func__);
        throw OFIException(EBUSY, err_str);
    }

    cout << "Name: " << hostname << endl;
    cout << "Src: " << (char*) fi->src_addr << "\" [" << fi->dest_addrlen << "]" << endl;
    cout << "Dst: \"" << (char*) fi->dest_addr << "\" [" << fi->dest_addrlen << "]" << endl;
    cout << "Format: " << fi->addr_format << endl;
    cout << "Prov: " << fi->fabric_attr->prov_name << endl;

/*    CTX->log()->info("{}() fi_getinfo: src_addr '{}' dest_addr '{}' domain_name '{}' fabric_name '{}' fabric_prov_name '{}' fabric_prov_version '{}'", __func__,
                     (char*) fi->src_addr,
                     (char*) fi->dest_addr,
                     fi->domain_attr->name,
                     fi->fabric_attr->name,
                     fi->fabric_attr->prov_version);
*/
    string host = reinterpret_cast<char*>(fi->dest_addr);
//    fi_freeinfo(fi);
//    fi_freeinfo(hints);

    auto sep_pos = host.find("://");
    if (sep_pos == string::npos) {
        auto err_str = fmt::format("{}() Unexpected format of resolved hostname: '{}'", __func__, host);
        throw OFIException(EBUSY, err_str);
    }

    // remove the address format specifier
    host.erase(0, sep_pos + 3);

    /**
    sep_pos = host.find(":");
    if (sep_pos == string::npos) {
        throw runtime_error("Unexpected format of resolved hostname: " + host);
    }

    host.erase(sep_pos, string::npos);
    **/

    return host;
}

} // namespace rpc
} // namespace gkfs

