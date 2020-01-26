/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GEKKOFS_UTIL_HPP
#define GEKKOFS_UTIL_HPP

#include <string>
#include <vector>

namespace gkfs {
    namespace util {
        void populate_hosts_file();

        void destroy_hosts_file();

        void create_pid_file();

        void remove_pid_file();

        std::vector<std::pair<std::string, std::string>> get_interf_ips();

        std::string get_pid_file_path();
    }
}

#endif //GEKKOFS_UTIL_HPP
