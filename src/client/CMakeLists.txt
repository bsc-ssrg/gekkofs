set(PRELOAD_SRC
    gkfs_functions.cpp
    hooks.cpp
    intercept.cpp
    logging.cpp
    open_file_map.cpp
    open_dir.cpp
    preload.cpp
    preload_context.cpp
    preload_util.cpp
    resolve.cpp
    ../global/path_util.cpp
    ../global/rpc/rpc_utils.cpp
    rpc/hg_rpcs.cpp
    rpc/ld_rpc_data_ws.cpp
    rpc/ld_rpc_management.cpp
    rpc/ld_rpc_metadentry.cpp
    syscalls/detail/syscall_info.c
    )
set(PRELOAD_HEADERS
    ../../include/client/gkfs_functions.hpp
    ../../include/config.hpp
    ../../include/client/env.hpp
    ../../include/client/hooks.hpp
    ../../include/client/intercept.hpp
    ../../include/client/logging.hpp
    ../../include/client/make_array.hpp
    ../../include/client/open_file_map.hpp
    ../../include/client/open_dir.hpp
    ../../include/client/preload.hpp
    ../../include/client/preload_context.hpp
    ../../include/client/preload_util.hpp
    ../../include/client/resolve.hpp
    ../../include/client/rpc/hg_rpcs.hpp
    ../../include/client/rpc/ld_rpc_management.hpp
    ../../include/client/rpc/ld_rpc_metadentry.hpp
    ../../include/client/rpc/ld_rpc_data_ws.hpp
    ../../include/client/syscalls/args.hpp
    ../../include/client/syscalls/decoder.hpp
    ../../include/client/syscalls/errno.hpp
    ../../include/client/syscalls/rets.hpp
    ../../include/client/syscalls/syscall.hpp
    ../../include/client/syscalls/detail/syscall_info.h
    ../../include/global/cmake_configure.hpp
    ../../include/global/chunk_calc_util.hpp
    ../../include/global/global_defs.hpp
    ../../include/global/path_util.hpp
    ../../include/global/rpc/rpc_types.hpp
    ../../include/global/rpc/rpc_utils.hpp
    )

add_library(gkfs_intercept SHARED ${PRELOAD_SRC} ${PRELOAD_HEADERS})

target_link_libraries(gkfs_intercept
    # internal
    metadata
    distributor
    env_util
    ofi_utils
    # external
    Syscall_intercept::Syscall_intercept
    dl
    mercury
    hermes
    fmt::fmt
    Boost::boost # needed for tokenizer header
    Threads::Threads
    Date::TZ
)

target_include_directories(gkfs_intercept
    PRIVATE
    ${ABT_INCLUDE_DIRS}
    )

install(TARGETS gkfs_intercept
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/gkfs
)

