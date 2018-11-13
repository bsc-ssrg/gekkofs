set(PRELOAD_SRC
    preload_context.cpp
    adafs_functions.cpp
    intcp_functions.cpp
    open_file_map.cpp
    open_dir.cpp
    passthrough.cpp
    preload.cpp
    resolve.cpp
    preload_util.cpp
    rpc/engine.cpp
    rpc/ld_rpc_management.cpp
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
    ../../include/global/rpc/rpc_types.hpp
    ../../include/global/rpc/rpc_utils.hpp
    ../../include/global/path_util.hpp
    ../../include/global/chunk_calc_util.hpp
    ../../include/preload/preload_context.hpp
    ../../include/preload/adafs_functions.hpp
    ../../include/preload/intcp_functions.hpp
    ../../include/preload/open_file_map.hpp
    ../../include/preload/open_dir.hpp
    ../../include/preload/passthrough.hpp
    ../../include/preload/preload.hpp
    ../../include/preload/resolve.hpp
    ../../include/preload/preload_util.hpp
    ../../include/preload/rpc/engine.hpp
    ../../include/preload/rpc/ld_rpc_management.hpp
    ../../include/preload/rpc/ld_rpc_data_ws.hpp
    ../../include/preload/rpc/ld_rpc_metadentry.hpp
    )

add_library(adafs_preload_client SHARED ${PRELOAD_SRC} ${PRELOAD_HEADERS})

target_link_libraries(adafs_preload_client
    # internal
    metadata
    distributor
    log_util
    margo_wrap
    # external
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
