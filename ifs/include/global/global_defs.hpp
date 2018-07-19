#ifndef IFS_GLOBAL_DEFS_HPP
#define IFS_GLOBAL_DEFS_HPP

#include <type_traits> //underlying_type

// These constexpr set the RPC's identity and which handler the receiver end should use
namespace hg_tag {
    constexpr auto fs_config = "ipc_srv_fs_config";
    constexpr auto minimal = "rpc_minimal";
    constexpr auto create = "rpc_srv_mk_node";
    constexpr auto access = "rpc_srv_access";
    constexpr auto stat = "rpc_srv_stat";
    constexpr auto remove = "rpc_srv_rm_node";
    constexpr auto decr_size = "rpc_srv_decr_size";
    constexpr auto update_metadentry = "rpc_srv_update_metadentry";
    constexpr auto get_metadentry_size = "rpc_srv_get_metadentry_size";
    constexpr auto update_metadentry_size = "rpc_srv_update_metadentry_size";
    constexpr auto get_dirents = "rpc_srv_get_dirents";
    constexpr auto write_data = "rpc_srv_write_data";
    constexpr auto read_data = "rpc_srv_read_data";
    constexpr auto trunc_data = "rpc_srv_trunc_data";
}

// typedefs
typedef unsigned long rpc_chnk_id_t;

template<typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) {
    return static_cast<typename std::underlying_type<E>::type>(e);
}

#endif //IFS_GLOBAL_DEFS_HPP
