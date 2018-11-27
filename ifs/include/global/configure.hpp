/**
 * Attention: This whole configfile is not in a final form! This will eventually be a plaintext config file.
 */
#ifndef FS_CONFIGURE_H
#define FS_CONFIGURE_H

#include "../../configure_public.hpp"

// Daemon path to auxiliary files
#define DAEMON_AUX_PATH "/tmp"

// If ACM time should be considered
#define ACMtime //unused
// XXX Should blocksize and chunksize be merged?
#define BLOCKSIZE 4194304 // in bytes 4MB
#define CHUNKSIZE 4194304 // in bytes 4MB

// What metadata is used TODO this has to be parametrized or put into a configuration file
#define MDATA_USE_ATIME false
#define MDATA_USE_MTIME false
#define MDATA_USE_CTIME false
#define MDATA_USE_UID false
#define MDATA_USE_GID false
#define MDATA_USE_LINK_CNT false
#define MDATA_USE_BLOCKS false
#define MDATA_USE_SIZE true // XXX to be added in ADAFS_DATA. currently on by default

// should permissions be checked when access() is called or discarded (disabled by default)
//#define CHECK_ACCESS
// If access permissions should be checked while opening a file (disabled by default)
//#define CHECK_ACCESS_DURING_OPEN
// If disabled, a file or directory is always presumed to be there (even if it is not). No check is executed (enabled by default)
#define DO_LOOKUP
/*
 * Zero buffer before read. This is relevant if sparse files are used.
 * If buffer is not zeroed, sparse regions contain invalid data.
 */
//#define ZERO_BUFFER_BEFORE_READ

// Write-ahead logging of rocksdb
//#define KV_WOL
// Optimize Key-Value store. Eventually, different modes will be available for different workloads. TODO
//#define KV_OPTIMIZE
// Optimize Key-Value store for tmpfs/ramdisk usage
//#define KV_OPTIMIZE_RAMDISK

// Buffer size for Rocksdb. A high number means that all entries are held in memory.
// However, when full the application blocks until **all** entries are flushed to disk.
//#define KV_WRITE_BUFFER 16384

// Margo and Argobots configuration

/*
 * Indicates the number of concurrent progress to drive I/O operations of chunk files to and from local file systems
 * The value is directly mapped to created Argobots xstreams, controlled in a single pool with ABT_snoozer scheduler
 */
#define DAEMON_IO_XSTREAMS 8
// Number of threads used for RPC and IPC handlers at the daemon
#define DAEMON_RPC_HANDLER_XSTREAMS 8
#define RPC_PORT 43000
#define RPC_TRIES 3
// rpc timeout to try again in milliseconds
#define RPC_TIMEOUT 180000
// enables timing of sending rpcs
//#define MARGO_FORWARD_TIMER
// sets the threshold in milliseconds when a log entry should be created
#define MARGO_FORWARD_TIMER_THRESHOLD 1000

//size of preallocated buffer to hold directory entries in rpc call
#define RPC_DIRENTS_BUFF_SIZE (8 * 1024 * 1024) // 8 mega

// Debug configurations
//#define RPC_TEST //unused

// environment prefixes
#define ENV_PREFIX "ADAFS_"

// Log
#define DEFAULT_PRELOAD_LOG_PATH "/tmp/adafs_preload.log"
#define DEFAULT_DAEMON_LOG_PATH "/tmp/adafs_daemon.log"

#define DEFAULT_PRELOAD_LOG_LEVEL 4 // info
#define DEFAULT_DAEMON_LOG_LEVEL 4 // info

#endif //FS_CONFIGURE_H
