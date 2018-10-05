cmake_minimum_required(VERSION 3.6)
project(ifs)

if(NOT CMAKE_COMPILER_IS_GNUCC)
    message(FATAL_ERROR "The choosen C compiler is not gcc and is not supported")
endif()
if(NOT CMAKE_COMPILER_IS_GNUCXX)
    message(FATAL_ERROR "The choosen C++ compiler is not g++ and is not supported")
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release
            CACHE STRING "Choose the type of build: Debug Release Memcheck" FORCE)
ENDIF (NOT CMAKE_BUILD_TYPE)
message("* Current build type is : ${CMAKE_BUILD_TYPE}")

# Compiler flags for various cmake build types
set(WARNINGS_FLAGS "-Wall -Wextra --pedantic -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -O3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${WARNINGS_FLAGS} -g -O0")
set(CMAKE_CXX_FLAGS_MEMCHECK "${WARNINGS_FLAGS} -g -O0 -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_MAINTAINER "${WARNINGS_FLAGS} -g -O0 -pg -no-pie")
mark_as_advanced(CMAKE_CXX_FLAGS_MAINTAINER)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})

set(CMAKE_EXPORT_COMPILE_COMMANDS 0)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Rocksdb dependencies
find_package(LZ4 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)
find_package(Snappy REQUIRED)
find_package(ZStd REQUIRED)
find_package(JeMalloc) # required if rocksdb has been build with jemalloc
find_package(RocksDB REQUIRED)
# margo dependencies
find_package(Mercury REQUIRED)
find_package(Abt REQUIRED)
find_package(Margo REQUIRED)
find_package(Syscall_intercept REQUIRED)

# boost dependencies, system is required for filesystem
find_package(Boost 1.53 REQUIRED
    COMPONENTS
    filesystem
    program_options
    )

find_package(Threads REQUIRED)

set(RPC_PROTOCOL "ofi+tcp" CACHE STRING "Communication plugin used for RPCs")
set_property(CACHE RPC_PROTOCOL PROPERTY STRINGS
   "bmi+tcp"
   "ofi+tcp"
   "ofi+verbs"
   "ofi+psm2"
   "cci+verbs"
)
message(STATUS "RPC protocol: '${RPC_PROTOCOL}'")
add_definitions(-DRPC_PROTOCOL="${RPC_PROTOCOL}")

option(SYMLINK_SUPPORT "Compile with support for symlinks" ON)
if(SYMLINK_SUPPORT)
    add_compile_definitions(HAS_SYMLINKS)
endif()

# Imported target
add_library(RocksDB INTERFACE IMPORTED GLOBAL)
target_link_libraries(RocksDB
    INTERFACE
    ${ROCKSDB_LIBRARIES}
    # rocksdb libs
    ${Snappy_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${BZIP2_LIBRARIES}
    ${ZSTD_LIBRARIES}
    ${LZ4_LIBRARIES}
)

if(${JeMalloc_FOUND})
    target_link_libraries(RocksDB
        INTERFACE
        ${JEMALLOC_LIBRARIES}
    )
endif()
# we cannot use target_include_directories with CMake < 3.11
set_target_properties(RocksDB
    PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${ROCKSDB_INCLUDE_DIRS}
)

add_library(spdlog INTERFACE)
# we cannot use target_include_directories with CMake < 3.11
set_target_properties(spdlog
    PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/include/extern"
)

add_library(fmt INTERFACE)
# we cannot use target_include_directories with CMake < 3.11
set_target_properties(fmt
    PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/include/extern/spdlog"
)

set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")

# define include directories that are relevant for all targets
include_directories(
    ${INCLUDE_DIR}
)

# Global components
add_subdirectory(src/global)
# Daemon
add_subdirectory(src/daemon)
# Client library
add_subdirectory(src/preload)
