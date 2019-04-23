/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  SPDX-License-Identifier: MIT
*/

#ifndef IOINTERCEPT_PRELOAD_HPP
#define IOINTERCEPT_PRELOAD_HPP

#include <client/preload_context.hpp>


#define EUNKNOWN (-1)

#define CTX PreloadContext::getInstance()

void init_ld_env_if_needed();

void init_preload() __attribute__((constructor));

void destroy_preload() __attribute__((destructor));


#endif //IOINTERCEPT_PRELOAD_HPP
