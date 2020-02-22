include(GkfsPythonTesting)

# ensure helper programs in the testing harness get built
add_subdirectory(harness)

gkfs_enable_python_testing(
    BINARY_DIRECTORIES ${CMAKE_BINARY_DIR}/src/daemon/
                       ${CMAKE_BINARY_DIR}/src/client/
                       ${CMAKE_BINARY_DIR}/tests/harness/
    LIBRARY_PREFIX_DIRECTORIES ${CMAKE_PREFIX_PATH}
)

add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND}
            --force-new-ctest-process
            --verbose
            --output-on-failure
)

gkfs_add_python_test(
    NAME test_directories
    PYTHON_VERSION 3.7
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMAND tests/test_directories.py
)
