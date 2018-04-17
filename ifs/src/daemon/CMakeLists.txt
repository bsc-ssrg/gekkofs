set(DAEMON_SRC
    ../global/rpc/rpc_utils.cpp
    ../global/global_func.cpp
    adafs_daemon.cpp
    adafs_ops/data.cpp
    adafs_ops/metadentry.cpp
    classes/fs_data.cpp
    classes/rpc_data.cpp
    classes/metadata.cpp
    db/db_util.cpp
    db/db_ops.cpp
    db/merge.cpp
    handler/h_metadentry.cpp
    handler/h_data.cpp
    handler/h_preload.cpp
    )
set(DAEMON_HEADERS
    ../../configure_public.hpp
    ../../include/global/configure.hpp
    ../../include/global/global_defs.hpp
    ../../include/global/global_func.hpp
    ../../include/global/rpc/ipc_types.hpp
    ../../include/global/rpc/rpc_types.hpp
    ../../include/global/rpc/rpc_utils.hpp
    ../../include/daemon/adafs_daemon.hpp
    ../../include/daemon/adafs_ops/data.hpp
    ../../include/daemon/adafs_ops/metadentry.hpp
    ../../include/daemon/classes/fs_data.hpp
    ../../include/daemon/classes/metadata.hpp
    ../../include/daemon/classes/rpc_data.hpp
    ../../include/daemon/db/db_ops.hpp
    ../../include/daemon/db/db_util.hpp
    ../../include/daemon/db/merge.hpp
    ../../include/daemon/handler/rpc_defs.hpp
    )
add_executable(adafs_daemon ${DAEMON_SRC} ${DAEMON_HEADERS})
target_link_libraries(adafs_daemon
    ${ROCKSDB_LIBRARIES}
    # rocksdb libs
    ${snappy_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${BZIP2_LIBRARIES}
    ${ZSTD_LIBRARIES}
    ${LZ4_LIBRARY}
    ${gflags_LIBRARIES}
    # margo libs
    ${ABT_LIBRARIES}
    ${ABT_SNOOZER_LIBRARIES}
    ${NA_LIB}
    ${MERCURY_LIBRARIES}
    ${MERCURY_UTIL_LIBRARIES}
    ${MARGO_LIBRARIES}
    ${Boost_LIBRARIES}
    Threads::Threads
    )
if (${JEMALLOC_FOUND})
    target_link_libraries(adafs_daemon ${JEMALLOC_LIBRARIES})
endif ()
