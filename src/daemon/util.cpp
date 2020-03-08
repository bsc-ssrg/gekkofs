/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/
#include <daemon/util.hpp>
#include <daemon/main.hpp>
#include <global/rpc/rpc_utils.hpp>
#include <global/exceptions.hpp>

#include <boost/filesystem.hpp>

#include <fstream>
#include <iostream>

extern "C" {
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
}

using namespace std;
namespace bfs = boost::filesystem;

void gkfs::util::populate_hosts_file() {
    const auto& hosts_file = GKFS_DATA->hosts_file();
    GKFS_DATA->spdlogger()->debug("{}() Populating hosts file: '{}'", __func__, hosts_file);
    ofstream lfstream(hosts_file, ios::out | ios::app);
    if (!lfstream) {
        throw runtime_error(
                fmt::format("Failed to open hosts file '{}': {}", hosts_file, strerror(errno)));
    }
    lfstream << fmt::format("{} {}", get_my_hostname(true), RPC_DATA->self_addr_str()) << std::endl;
    if (!lfstream) {
        throw runtime_error(
                fmt::format("Failed to write on hosts file '{}': {}", hosts_file, strerror(errno)));
    }
    lfstream.close();
}

void gkfs::util::destroy_hosts_file() {
    std::remove(GKFS_DATA->hosts_file().c_str());
}

/**
 * Creates a pid file and checks that a daemon is not already running
 * @param rpc_self_addr
 */
void gkfs::util::create_pid_file() {

    auto pid_path = get_pid_file_path();

    // check if a pid file already exists for network interface (Only one daemon per network interface )
//    ifstream ifs(pid_path, ::ifstream::in);

    // get own pid and write pid file designated path
    auto my_pid = getpid();
    if (my_pid == -1) {
        throw runtime_error(fmt::format("Unable to get pid for pid file"));
    }
    ofstream ofs(pid_path, ::ofstream::trunc);
    if (ofs) {
        ofs << to_string(my_pid);
    } else {
        throw runtime_error(fmt::format("Unable to create pid file at '{}'", pid_path));
    }
    ofs.close();
}

void gkfs::util::remove_pid_file() {
    auto pid_path = get_pid_file_path();
    bfs::remove(pid_path);
}

/**
 * creates a pid file name and path depending on used interface and port (if applicable)
 * If ofi+sockets is used the pid file will have this form: '<prefix_path>_<network_interface>_<port>.pid'
 * because multiple daemons can be started on multiple ports even if its the same interface
 * If ofi+ib or ofi+psm2 is used the pid file will have this form: '<prefix_path>_<network_interface>.pid'
 * because native Infiniband or OmniPath does not use the concept of ports
 * @return <string> path to pid file
 */
string gkfs::util::get_pid_file_path() {
    // get used network interface for use in pid file string
    auto interf_ips = get_interf_ips();
    string used_interf{};
    for (auto& e : interf_ips) {
        if (RPC_DATA->self_addr_str().find(e.second) != string::npos) {
            used_interf = e.first;
        }
    }
    string pid_path{};
    auto protocol_substr = "ofi+sockets://"s;
    auto sockets_idx = RPC_DATA->self_addr_str().find("ofi+sockets://");
    if (sockets_idx != string::npos) {
        // in mercury the RPC_PROTOCOL is always the last item in the self_addr
        auto port_idx = RPC_DATA->self_addr_str().find(':', sockets_idx + protocol_substr.length()) + 1;
        auto port = RPC_DATA->self_addr_str().substr(port_idx, RPC_DATA->self_addr_str().length() - port_idx);
        // get port from self addr
        pid_path = fmt::format("{}_{}_{}.pid", gkfs_config::daemon_pid_path, used_interf, port);
    } else
        pid_path = fmt::format("{}_{}.pid", gkfs_config::daemon_pid_path, used_interf);

    return pid_path;
}

/**
 * Gets all network interfaces and its ips. Returns the associated info in a list.
 * @return list of pairs: (interface, ips)
 */
vector<pair<string, string>> gkfs::util::get_interf_ips() {

    vector<pair<string,string>> interfaces{};
    auto addrs = make_unique<struct ifaddrs>();
    // wrapper var because ** cannot be an rvalue
    auto addrs_raw_ptr = addrs.get();
    auto err = getifaddrs(&addrs_raw_ptr);
    if (err != 0) {
        auto err_str = fmt::format("Error getting network interfaces and ips for pid file. errno: {}", errno);
        throw gkfs::error::NetworkAddrException(err_str);
    }
    // helper variable
    auto addr = addrs_raw_ptr;
    while(addr) {
        if (addr->ifa_addr && addr->ifa_addr->sa_family == AF_INET) {
            auto sa = reinterpret_cast<struct sockaddr_in*>(addr->ifa_addr);
            auto ip_str = inet_ntoa(sa->sin_addr);
            interfaces.emplace_back(pair<string, string>(addr->ifa_name, ip_str));
            GKFS_DATA->spdlogger()->debug("{}() Found network interface and IP: '{}' -> '{}'", __func__, addr->ifa_name,
                                          ip_str);
        }
        addr = addr->ifa_next;
    }
    return interfaces;
}
