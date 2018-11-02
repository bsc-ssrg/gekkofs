add_library(metadata_db STATIC)

target_sources(metadata_db
    PUBLIC
    ${INCLUDE_DIR}/daemon/backend/metadata/db.hpp
    ${INCLUDE_DIR}/daemon/backend/exceptions.hpp
    PRIVATE
    ${INCLUDE_DIR}/global/path_util.hpp
    ${INCLUDE_DIR}/global/metadata.hpp
    ${INCLUDE_DIR}/daemon/backend/metadata/merge.hpp
    ${CMAKE_CURRENT_LIST_DIR}/merge.cpp
    ${CMAKE_CURRENT_LIST_DIR}/db.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../../global/metadata.cpp
    )

target_link_libraries(metadata_db
    RocksDB
    spdlog
)
