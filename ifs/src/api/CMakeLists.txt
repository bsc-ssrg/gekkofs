cmake_minimum_required(VERSION 3.6)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

set(WARNINGS_FLAGS "-Wall -Wextra --pedantic -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -O3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${WARNINGS_FLAGS} -g -O0")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${WARNINGS_FLAGS} -g -O0")

add_library(gekkofs_api SHARED "")
add_library(gekkofs_capi SHARED "")

target_sources(gekkofs_api
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/gekkofs.h
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/libgekkofs-api.cpp
    )

target_sources(gekkofs_capi
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/gekkofs.h
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/libgekkofs-api.c
    )
