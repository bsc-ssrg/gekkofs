/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/


#ifndef GEKKOFS_CLIENT_FORWARD_METADATA_HPP
#define GEKKOFS_CLIENT_FORWARD_METADATA_HPP

#include <string>

/* Forward declaration */
namespace gkfs {
namespace filemap {
class OpenDir;
}
namespace metadata {
struct MetadentryUpdateFlags;
class Metadata;
}

namespace rpc {

int forward_create(const std::string& path, mode_t mode);

int forward_stat(const std::string& path, std::string& attr);

int forward_remove(const std::string& path, bool remove_metadentry_only, ssize_t size);

int forward_decr_size(const std::string& path, size_t length);

int forward_update_metadentry(const std::string& path, const gkfs::metadata::Metadata& md,
                              const gkfs::metadata::MetadentryUpdateFlags& md_flags);

int forward_update_metadentry_size(const std::string& path, size_t size, off64_t offset, bool append_flag,
                                   off64_t& ret_size);

int forward_get_metadentry_size(const std::string& path, off64_t& ret_size);

void forward_get_dirents(gkfs::filemap::OpenDir& open_dir);

#ifdef HAS_SYMLINKS

int forward_mk_symlink(const std::string& path, const std::string& target_path);

#endif


} // namespace rpc
} // namespace gkfs

#endif //GEKKOFS_CLIENT_FORWARD_METADATA_HPP
