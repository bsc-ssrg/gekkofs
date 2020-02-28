# - Try to find LIBFABRIC
# Once done this will define
#  LIBFABRIC_FOUND - System has LIBFABRIC
#  LIBFABRIC_INCLUDE_DIRS - The LIBFABRIC include directories
#  LIBFABRIC_LIBRARIES - The libraries needed to use LIBFABRIC

find_package(PkgConfig)
pkg_check_modules(PC_LIBFABRIC libfabric QUIET)

find_path(LIBFABRIC_INCLUDE_DIR rdma/fabric.h
    HINTS ${PC_LIBFABRIC_INCLUDEDIR} ${PC_LIBFABRIC_INCLUDE_DIRS})

find_library(LIBFABRIC_LIBRARY NAMES fabric
    HINTS ${PC_LIBFABRIC_LIBDIR} ${PC_LIBFABRIC_LIBRARY_DIRS})

if(LIBFABRIC_INCLUDE_DIR AND EXISTS "${LIBFABRIC_INCLUDE_DIR}/rdma/fabric.h")
    file(STRINGS "${LIBFABRIC_INCLUDE_DIR}/rdma/fabric.h" FABRIC_H REGEX "^#define FI_MAJOR_VERSION [0-9]+$")
    string(REGEX MATCH "[0-9]+$" LIBFABRIC_VERSION_MAJOR "${FABRIC_H}")

    file(STRINGS "${LIBFABRIC_INCLUDE_DIR}/rdma/fabric.h" FABRIC_H REGEX "^#define FI_MINOR_VERSION [0-9]+$")
    string(REGEX MATCH "[0-9]+$" LIBFABRIC_VERSION_MINOR "${FABRIC_H}")
    set(LIBFABRIC_VERSION_STRING "${LIBFABRIC_VERSION_MAJOR}.${LIBFABRIC_VERSION_MINOR}")

    set(LIBFABRIC_MAJOR_VERSION "${LIBFABRIC_VERSION_MAJOR}")
    set(LIBFABRIC_MINOR_VERSION "${LIBFABRIC_VERSION_MINOR}")
endif()


set(LIBFABRIC_INCLUDE_DIRS ${LIBFABRIC_INCLUDE_DIR})
set(LIBFABRIC_LIBRARIES ${LIBFABRIC_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBFABRIC REQUIRED_VARS
    LIBFABRIC_INCLUDE_DIR LIBFABRIC_LIBRARY
    VERSION_VAR LIBFABRIC_VERSION_STRING)

mark_as_advanced(LIBFABRIC_INCLUDE_DIR LIBFABRIC_LIBRARY)