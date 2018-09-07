set(PRELOAD_SRC
    preload_context.cpp
    adafs_functions.cpp
    intercept.cpp
    hooks.cpp
    margo_ipc.cpp
    open_file_map.cpp
    open_dir.cpp
    preload.cpp
    resolve.cpp
    preload_util.cpp
    rpc/ld_rpc_data_ws.cpp
    rpc/ld_rpc_metadentry.cpp
    ../global/rpc/rpc_utils.cpp
    ../global/global_func.cpp
    ../global/path_util.cpp
    )
set(PRELOAD_HEADERS
    ../../configure_public.hpp
    ../../include/global/configure.hpp
    ../../include/global/global_defs.hpp
    ../../include/global/global_func.hpp
    ../../include/global/rpc/ipc_types.hpp
    ../../include/global/rpc/rpc_types.hpp
    ../../include/global/rpc/rpc_utils.hpp
    ../../include/global/path_util.hpp
    ../../include/global/chunk_calc_util.hpp
    ../../include/preload/preload_context.hpp
    ../../include/preload/adafs_functions.hpp
    ../../include/preload/intercept.hpp
    ../../include/preload/syscall_names.hpp
    ../../include/preload/hooks.hpp
    ../../include/preload/margo_ipc.hpp
    ../../include/preload/open_file_map.hpp
    ../../include/preload/open_dir.hpp
    ../../include/preload/preload.hpp
    ../../include/preload/resolve.hpp
    ../../include/preload/preload_util.hpp
    ../../include/preload/rpc/ld_rpc_data_ws.hpp
    ../../include/preload/rpc/ld_rpc_metadentry.hpp
    )

add_library(adafs_preload_client SHARED ${PRELOAD_SRC} ${PRELOAD_HEADERS})

target_link_libraries(adafs_preload_client
    # internal
    metadata
    distributor
    log_util
    # external
    Syscall_intercept::Syscall_intercept
    dl
    ${ABT_LIBRARIES}
    mercury
    ${MARGO_LIBRARIES}
    Boost::boost # needed for tokenizer header
    Threads::Threads
)

target_include_directories(adafs_preload_client
    PRIVATE
    ${ABT_INCLUDE_DIRS}
    ${MARGO_INCLUDE_DIRS}
    )
