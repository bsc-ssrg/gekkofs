/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <string>
#include <stdexcept>

#ifndef GEKKOFS_EXCEPTIONS_HPP
#define GEKKOFS_EXCEPTIONS_HPP

namespace gkfs {
    namespace error {
        class NetworkAddrException: public std::runtime_error {
        public:
            NetworkAddrException(const std::string & s) : std::runtime_error(s) {};
        };
    }
}

#endif //GEKKOFS_EXCEPTIONS_HPP
