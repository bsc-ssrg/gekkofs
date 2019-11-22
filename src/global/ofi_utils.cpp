/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include "global/ofi_utils.hpp"
#include <rdma/fabric.h>
#include <stdexcept>
#include <cstring>

//TODO remove
#include <iostream>

using namespace std;


std::string ofi_gethostbyname(const std::string& name, const std::string& provider) {
    struct fi_info* hints;
    struct fi_info* fi;
    hints = fi_allocinfo();
    if (!hints) {
        throw runtime_error("Failed to allocate hints struct for fi_getinfo call");
    }
    // hints is passed to fi_getinfo as const struct, so the cast here is safe
    hints->fabric_attr->prov_name = const_cast<char*>(provider.c_str());
    hints->addr_format = FI_ADDR_STR;
    hints->ep_attr->protocol = FI_PROTO_PSMX2;

    auto err = fi_getinfo(FI_VERSION(FI_MAJOR_VERSION, FI_MINOR_VERSION), name.c_str(), nullptr, 0, hints, &fi);
    if (err) {
        throw runtime_error("Failed to get host by name through fi_getinfo: "s + strerror(-err));
    }
    if (fi == nullptr) {
        throw runtime_error("Failed to get host by name through fi_getinfo: no results");
    }

    if (fi->next != nullptr) {
        while (fi->next) {
            cerr << "Name: " << name << endl;
            cerr << "Src: " << (char*) fi->src_addr << "\" [" << fi->dest_addrlen << "]" << endl;
            cerr << "Dst: \"" << (char*) fi->dest_addr << "\" [" << fi->dest_addrlen << "]" << endl;
            cerr << "Format: " << fi->addr_format << endl;
            cerr << "Prov: " << fi->fabric_attr->prov_name << endl;
            fi = fi->next;
        }
        throw runtime_error("fi_getinfo: multiple results found for specific hostname");
    }

    cout << "Name: " << name << endl;
    cout << "Src: " << (char*) fi->src_addr << "\" [" << fi->dest_addrlen << "]" << endl;
    cout << "Dst: \"" << (char*) fi->dest_addr << "\" [" << fi->dest_addrlen << "]" << endl;
    cout << "Format: " << fi->addr_format << endl;
    cout << "Prov: " << fi->fabric_attr->prov_name << endl;

    string host = reinterpret_cast<char*>(fi->dest_addr);
    //fi_freeinfo(fi);
    //fi_freeinfo(hints);

    auto sep_pos = host.find("://");
    if (sep_pos == string::npos) {
        throw runtime_error("Unexpected format of resolved hostname: " + host);
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

