add_subdirectory(backend/metadata)
add_subdirectory(backend/data)

set(DAEMON_SRC
    ../global/rpc/rpc_util.cpp
    ../global/path_util.cpp
    daemon.cpp
    util.cpp
    ops/metadentry.cpp
    classes/fs_data.cpp
    classes/rpc_data.cpp
    handler/srv_metadata.cpp
    handler/srv_data.cpp
    handler/srv_management.cpp
    )
set(DAEMON_HEADERS
    ../../include/config.hpp
    ../../include/version.hpp
    ../../include/global/cmake_configure.hpp
    ../../include/global/global_defs.hpp
    ../../include/global/rpc/rpc_types.hpp
    ../../include/global/rpc/rpc_util.hpp
    ../../include/global/path_util.hpp
    ../../include/daemon/daemon.hpp
    ../../include/daemon/util.hpp
    ../../include/daemon/ops/metadentry.hpp
    ../../include/daemon/classes/fs_data.hpp
    ../../include/daemon/classes/rpc_data.hpp
    ../../include/daemon/handler/rpc_defs.hpp
    ../../include/daemon/handler/rpc_util.hpp
    )
add_executable(gkfs_daemon ${DAEMON_SRC} ${DAEMON_HEADERS})
target_link_libraries(gkfs_daemon
    # internal libs
    metadata
    metadata_db
    storage
    distributor
    log_util
    env_util
    spdlog
    fmt::fmt
    # margo libs
    ${ABT_LIBRARIES}
    mercury
    ${MARGO_LIBRARIES}
    # others
    Boost::boost # needed for tokenizer header
    Boost::program_options
    Boost::filesystem
    Threads::Threads
    )

target_include_directories(gkfs_daemon
    PRIVATE
    ${ABT_INCLUDE_DIRS}
    ${MARGO_INCLUDE_DIRS}
    )

install(TARGETS gkfs_daemon
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
