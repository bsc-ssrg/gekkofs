
#ifndef IFS_IPC_TYPES_HPP
#define IFS_IPC_TYPES_HPP

extern "C" {
#include <mercury.h>
#include <mercury_types.h>
#include <mercury_proc_string.h>
#include <margo.h>
}

MERCURY_GEN_PROC(ipc_err_out_t, ((hg_int32_t) (err))) // generic return type

MERCURY_GEN_PROC(ipc_config_in_t, ((hg_int32_t) (dummy))) // XXX remove that.

MERCURY_GEN_PROC(ipc_config_out_t, ((hg_const_string_t) (mountdir))
        ((hg_const_string_t) (rootdir)) \
((hg_bool_t) (atime_state)) \
((hg_bool_t) (mtime_state)) \
((hg_bool_t) (ctime_state)) \
((hg_bool_t) (uid_state)) \
((hg_bool_t) (gid_state)) \
((hg_bool_t) (link_cnt_state)) \
((hg_bool_t) (blocks_state)) \
((hg_uint32_t) (uid)) \
((hg_uint32_t) (gid)) \
((hg_const_string_t) (hosts_raw)) \
((hg_uint64_t) (host_id)) \
((hg_uint64_t) (host_size)))

#endif //IFS_IPC_TYPES_HPP
