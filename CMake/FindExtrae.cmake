# Try to find Extrae headers and library.
#
# Usage of this module as follows:
#
#     find_package(Extrae)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
# Variables defined by this module:
#
#  EXTRAE_FOUND               System has Extrae library/headers.
#  EXTRAE_LIBRARIES           The Extrae library.
#  EXTRAE_INCLUDE_DIRS        The location of Extrae headers.


find_path(EXTRAE_INCLUDE_DIR
    NAMES "extrae.h"
    HINTS "$ENV{EXTRAE_HOME}/include"
)

find_library(EXTRAE_LIBRARY
    NAMES "pttrace"
    HINTS "$ENV{EXTRAE_HOME}/lib"
)


if(EXTRAE_INCLUDE_DIR AND EXISTS "${EXTRAE_INCLUDE_DIR}/extrae_version.h")
    file(READ "${EXTRAE_INCLUDE_DIR}/extrae_version.h" EXTRAE_VERSION_HEADER)

    string(REGEX MATCH "#define EXTRAE_MAJOR ([0-9]+)" _ ${EXTRAE_VERSION_HEADER})
    set(EXTRAE_VERSION_MAJOR ${CMAKE_MATCH_1})

    string(REGEX MATCH "#define EXTRAE_MINOR ([0-9]+)" _ ${EXTRAE_VERSION_HEADER})
    set(EXTRAE_VERSION_MINOR ${CMAKE_MATCH_1})

    string(REGEX MATCH "#define EXTRAE_MICRO ([0-9]+)" _ ${EXTRAE_VERSION_HEADER})
    set(EXTRAE_VERSION_MICRO ${CMAKE_MATCH_1})

    set(EXTRAE_VERSION_STRING "${EXTRAE_VERSION_MAJOR}.${EXTRAE_VERSION_MINOR}.${EXTRAE_VERSION_MICRO}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Extrae
    REQUIRED_VARS EXTRAE_LIBRARY EXTRAE_INCLUDE_DIR
    VERSION_VAR EXTRAE_VERSION_STRING
)

mark_as_advanced(
        EXTRAE_LIBRARY
        EXTRAE_INCLUDE_DIR
)

set(EXTRAE_INCLUDE_DIRS ${EXTRAE_INCLUDE_DIR})
set(EXTRAE_LIBRARIES ${EXTRAE_LIBRARY})

if(Extrae_FOUND)
    add_library(Extrae::Extrae INTERFACE IMPORTED)
    set_target_properties(Extrae::Extrae PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${EXTRAE_INCLUDE_DIRS}"
    )
    target_link_libraries(Extrae::Extrae
        INTERFACE
        ${EXTRAE_LIBRARIES}
    )
endif()