/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GKFS_OFI_UTIL_HPP
#define GKFS_OFI_UTIL_HPP

#include <string>

std::string ofi_gethostbyname(const std::string& name, const std::string& prov);

#endif //GKFS_OFI_UTIL_HPP

