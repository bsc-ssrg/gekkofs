add_library(storage STATIC)

target_sources(storage
    PUBLIC
    ${INCLUDE_DIR}/daemon/backend/data/chunk_storage.hpp
    ${INCLUDE_DIR}/global/global_defs.hpp
    PRIVATE
    ${INCLUDE_DIR}/global/path_util.hpp
    ${INCLUDE_DIR}/extern/spdlog/spdlog.h
    ${CMAKE_CURRENT_LIST_DIR}/chunk_storage.cpp
    )

target_link_libraries(storage
    PRIVATE
    spdlog
    Boost::filesystem
    ${ABT_LIBRARIES}
    )

target_include_directories(storage
    PRIVATE
    ${ABT_INCLUDE_DIRS}
    )
