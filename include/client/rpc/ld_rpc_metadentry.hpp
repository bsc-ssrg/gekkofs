
#ifndef IFS_PRELOAD_C_METADENTRY_HPP
#define IFS_PRELOAD_C_METADENTRY_HPP

#include <string>

/* Forward declaration */
struct MetadentryUpdateFlags;
class OpenDir;
class Metadata;

namespace rpc_send {


int mk_node(const std::string& path, mode_t mode, fuid_t& fuid);

int stat(const std::string& path, std::string& attr);

int rm_node(const std::string& path, const fuid_t fuid, const bool remove_metadentry_only);

int decr_size(const std::string& path, size_t length);

int update_metadentry(const std::string& path, const Metadata& md, const MetadentryUpdateFlags& md_flags);

int update_metadentry_size(const std::string& path, size_t size, off64_t offset, bool append_flag,
                                    off64_t& ret_size);

int get_metadentry_size(const std::string& path, off64_t& ret_size);

void get_dirents(OpenDir& open_dir);

#ifdef HAS_SYMLINKS
int mk_symlink(const std::string& path, const std::string& target_path);
#endif


} // end namespace rpc_send

#endif //IFS_PRELOAD_C_METADENTRY_HPP
