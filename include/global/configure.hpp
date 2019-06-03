/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

/**
 * Attention: This whole configfile is not in a final form! This will eventually be a plaintext config file.
 */
#ifndef FS_CONFIGURE_H
#define FS_CONFIGURE_H

// Daemon path to auxiliary files
#define DAEMON_AUX_PATH "/tmp"

#define CHUNKSIZE  1024*512 // in bytes 512KB

// What metadata is used TODO this has to be parametrized or put into a configuration file
#define MDATA_USE_ATIME false
#define MDATA_USE_MTIME false
#define MDATA_USE_CTIME false
#define MDATA_USE_LINK_CNT false
#define MDATA_USE_BLOCKS false

/*
 * Zero buffer before read. This is relevant if sparse files are used.
 * If buffer is not zeroed, sparse regions contain invalid data.
 */
//#define ZERO_BUFFER_BEFORE_READ

// Write-ahead logging of rocksdb
#define KV_WOL false

// Buffer size for Rocksdb. A high number means that all entries are held in memory.
// However, when full the application blocks until **all** entries are flushed to disk.
//#define KV_WRITE_BUFFER 16384

// Margo and Argobots configuration

/*
 * Indicates the number of concurrent progress to drive I/O operations of chunk files to and from local file systems
 * The value is directly mapped to created Argobots xstreams, controlled in a single pool with ABT_snoozer scheduler
 */
#define DAEMON_IO_XSTREAMS 16
// Number of threads used for RPC handlers at the daemon
#define DAEMON_RPC_HANDLER_XSTREAMS 16
#define DEFAULT_RPC_PORT 4433
#define RPC_TRIES 3
// rpc timeout to try again in milliseconds
#define RPC_TIMEOUT 180000

//size of preallocated buffer to hold directory entries in rpc call
#define RPC_DIRENTS_BUFF_SIZE (16 * 1024 * 1024) // 8 mega

// environment prefixes
#define ENV_PREFIX "GKFS_"

// Log
#define DEFAULT_PRELOAD_LOG_PATH "/tmp/gkfs_preload.log"
#define DEFAULT_DAEMON_LOG_PATH "/tmp/gkfs_daemon.log"

#define DEFAULT_PRELOAD_LOG_LEVEL 4 // info
#define DEFAULT_DAEMON_LOG_LEVEL 4 // info

#endif //FS_CONFIGURE_H
