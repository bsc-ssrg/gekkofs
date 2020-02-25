/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef IO_COMMANDS_HPP
#define IO_COMMANDS_HPP

// forward declare CLI::App
namespace CLI { struct App; }

void
mkdir_init(CLI::App& app);

void
open_init(CLI::App& app);

void
opendir_init(CLI::App& app);

void
read_init(CLI::App& app);

void
readdir_init(CLI::App& app);

void
rmdir_init(CLI::App& app);

void
stat_init(CLI::App& app);

void
write_init(CLI::App& app);

#endif // IO_COMMANDS_HPP
