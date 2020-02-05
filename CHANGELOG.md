# Changelog
All notable changes to GekkoFS project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.6.2] - 2019-10-07
## Added
 - Paths inside kernel pseudo filesystems (`/sys`, `/proc`) are forwarded directly to the kernel and internal path resolution will be skipped. Be aware that also paths  like  `/sys/../tmp/gkfs_mountpoint/asd` will be forwarded to the kernel
 - Added new Cmake flag `CREATE_CHECK_PARENTS` to controls if the existance of the parent node needs to be checked during the creation of a child node.
## Changed
 - Daemon logs for RPC handlers have been polished
 - Updated Margo, Mercury and Libfabric dependencies
## Fixed
 - mk_node RPC wasn't propagating errors correctly from daemons
 - README has been improoved and got some minor fixes
 - fix wrong path in log call for mk_symlink function

## [0.6.1] - 2019-09-17
## Added
 - Added new Cmake flag `LOG_SYSCALLS` to enable/disable syscall logging.
 - Intercept the 64 bit version of `getdents`.
 - Added debian-based docker image.
## Changed
 - Disable syscalls logging by default
 - Update Mercury, RocksDB and Libfabric dependencies
## Fixed
 - Fix read at the end of file.
 - Don't create log file when using `--version`/`--help` cli flags.
 - On some systems LD_PRELOAD used on /bin/bash binary was not working.
 - Missing definition of `loff_t` on new version of GCC.

## [0.6.0] - 2019-07-26
## Added
- Add compile time option to disable shared memory communication `-DUSE_SHM:BOOL=OFF`
## Changed
- Deamons does not store anymore information about the others deamons.
- Improoved error handling on deamon initialization
- Decreased RPC timeout 3min -> 3sec
- Update 3rd party dependencies
## Removed
- PID file is not used anymore, we use only the new `hosts file` for out of bound communication
- Dropped CCI plugin support
- Dropped hostname-suffix cli option
- Dropped port cli option (use `--listen` instead)
- It is not needed anymore to pass hosts information to deamons, thus the `--hosts` cli have been removed
## Fixed
- Errors on get_dirents RPC are now reported back to clients
- Write errors happenig on deamons are now reported back to clients
- number overflow on lseek didn't allow to use seek on huge files

## [0.5.0] - 2019-04-29
## Changed
- Intercept I/O syscalls instead of GlibC function using [syscall intercept library](https://github.com/pmem/syscall_intercept)

## [0.4.0] - 2019-04-18
First GekkoFS pubblic release

This version provides a client library that uses GLibC I/O function interception.

## [0.3.1] - 2018-03-04
### Changed
- Read-write process improved. @Marc vef
- Improved Filemap. @Marc Vef
