cmake_minimum_required(VERSION 3.6)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_library(gfsctl SHARED)

target_sources(gfsctl
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/gfsctl.h
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/libgfsctl.cpp
    )
