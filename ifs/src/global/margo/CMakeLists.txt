add_library(margo_wrap STATIC)
set_property(TARGET margo_wrap PROPERTY POSITION_INDEPENDENT_CODE ON)
target_sources(margo_wrap
    PUBLIC
    ${INCLUDE_DIR}/global/margo/engine.hpp
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/engine.cpp
    ${CMAKE_CURRENT_LIST_DIR}/endpoint.cpp
    ${INCLUDE_DIR}/global/margo/endpoint.hpp
    ${CMAKE_CURRENT_LIST_DIR}/errors.cpp
    ${INCLUDE_DIR}/global/margo/errors.hpp
    )
target_link_libraries(margo_wrap
    ${ABT_LIBRARIES}
    mercury
    ${MARGO_LIBRARIES}
    )
