//
// Created by evie on 9/12/17.
//

#ifndef IFS_IPC_TYPES_HPP
#define IFS_IPC_TYPES_HPP

#include <preload/preload.hpp>

MERCURY_GEN_PROC(ipc_err_out_t, ((hg_int32_t) (err))) // generic return type

MERCURY_GEN_PROC(ipc_config_in_t, ((hg_int32_t) (dummy))) // XXX remove that.

MERCURY_GEN_PROC(ipc_config_out_t, ((hg_const_string_t) (mountdir))
        ((hg_const_string_t) (rootdir)) \
((hg_bool_t) (atime_state)) \
((hg_bool_t) (mtime_state)) \
((hg_bool_t) (ctime_state)) \
((hg_bool_t) (uid_state)) \
((hg_bool_t) (gid_state)) \
((hg_bool_t) (inode_no_state)) \
((hg_bool_t) (link_cnt_state)) \
((hg_bool_t) (blocks_state)) \
((hg_uint32_t) (uid)) \
((hg_uint32_t) (gid)) \
((hg_const_string_t) (hosts_raw)) \
((hg_uint64_t) (host_id)) \
((hg_uint64_t) (host_size)))


MERCURY_GEN_PROC(ipc_open_in_t, ((hg_const_string_t) (path))
        ((hg_int32_t) (flags)) \
((hg_uint32_t) (mode)))

MERCURY_GEN_PROC(ipc_stat_in_t, ((hg_const_string_t) (path)))

MERCURY_GEN_PROC(ipc_stat_out_t, ((hg_int32_t) (err))
        ((hg_const_string_t) (db_val)))

MERCURY_GEN_PROC(ipc_unlink_in_t, ((hg_const_string_t) (path)))

// data
MERCURY_GEN_PROC(ipc_read_data_in_t,
                 ((hg_const_string_t) (path))\
((hg_size_t) (size))\
((int64_t) (offset))\
((hg_bulk_t) (bulk_handle)))

MERCURY_GEN_PROC(ipc_data_out_t,
                 ((int32_t) (res))\
((hg_size_t) (io_size)))

MERCURY_GEN_PROC(ipc_write_data_in_t,
                 ((hg_const_string_t) (path))\
((hg_size_t) (size))\
((int64_t) (offset))\
((hg_bool_t) (append))\
((hg_bulk_t) (bulk_handle)))

#endif //IFS_IPC_TYPES_HPP
