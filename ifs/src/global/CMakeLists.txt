add_library(distributor STATIC)
set_property(TARGET distributor PROPERTY POSITION_INDEPENDENT_CODE ON)
target_sources(distributor
    PUBLIC
    ${INCLUDE_DIR}/global/rpc/distributor.hpp
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/rpc/distributor.cpp
    )

add_library(log_util STATIC)
set_property(TARGET log_util PROPERTY POSITION_INDEPENDENT_CODE ON)
target_sources(log_util
    PUBLIC
    ${INCLUDE_DIR}/global/log_util.hpp
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/log_util.cpp
    )
target_link_libraries(log_util
    spdlog
    )

add_library(metadata STATIC)
set_property(TARGET metadata PROPERTY POSITION_INDEPENDENT_CODE ON)
target_sources(metadata
    PUBLIC
    ${INCLUDE_DIR}/global/metadata.hpp
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/metadata.cpp
    )
target_link_libraries(metadata
    fmt
    )

add_library(margo_wrap STATIC)
set_property(TARGET margo_wrap PROPERTY POSITION_INDEPENDENT_CODE ON)
target_sources(margo_wrap
    PUBLIC
    ${INCLUDE_DIR}/global/margo.hpp
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/margo.cpp
    )
target_link_libraries(margo_wrap
    ${ABT_LIBRARIES}
    mercury
    ${MARGO_LIBRARIES}
    )
