add_library(margo_wrap STATIC)
set_property(TARGET margo_wrap PROPERTY POSITION_INDEPENDENT_CODE ON)
target_sources(margo_wrap
    PUBLIC
    ${INCLUDE_DIR}/global/margo/engine.hpp
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/engine.cpp
    )
target_link_libraries(margo_wrap
    ${ABT_LIBRARIES}
    mercury
    ${MARGO_LIBRARIES}
    )
