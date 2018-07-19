find_library(Snappy_LIBRARY
        NAMES snappy
        HINTS ${ADAFS_DEPS_INSTALL}
)

find_path(Snappy_INCLUDE_DIR
    NAMES snappy.h
    HINTS ${ADAFS_DEPS_INSTALL}
)

set(Snappy_LIBRARIES ${Snappy_LIBRARY})
set(Snappy_INCLUDE_DIRS ${Snappy_INCLUDE_DIR})


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(snappy DEFAULT_MSG Snappy_LIBRARY Snappy_INCLUDE_DIR)

mark_as_advanced(
        Snappy_LIBRARY
        Snappy_INCLUDE_DIR
)
