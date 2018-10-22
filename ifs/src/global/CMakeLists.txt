add_library(distributor STATIC)
set_property(TARGET distributor PROPERTY POSITION_INDEPENDENT_CODE ON)
target_sources(distributor
    PUBLIC
    ${INCLUDE_DIR}/global/rpc/distributor.hpp
    ${INCLUDE_DIR}/global/global_defs.hpp
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
