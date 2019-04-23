/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  SPDX-License-Identifier: MIT
*/

#include <global/global_func.hpp>

using namespace std;

string daemon_pid_path() {
    return (DAEMON_AUX_PATH + "/gkfs_daemon.pid"s);
}
