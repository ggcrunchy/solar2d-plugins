# Blosc - Blocked Shuffling and Compression Library
#
# Copyright (c) 2021  The Blosc Developers <blosc@blosc.org>
# https://blosc.org
# License: BSD 3-Clause (see LICENSE.txt)
#
# See LICENSE.txt for details about copyright and rights to use.

# sources
set(SOURCES ${SOURCES} ${PROJECT_SOURCE_DIR}/plugins/filters/bytedelta/bytedelta.c PARENT_SCOPE)

if(BUILD_TESTS)
    # targets
    add_executable(test_bytedelta test_bytedelta.c)
    # Define the BLOSC_TESTING symbol so normally-hidden functions
    # are available to the test programs.
    set_property(
            TARGET test_bytedelta
            APPEND PROPERTY COMPILE_DEFINITIONS BLOSC_TESTING)
    target_link_libraries(test_bytedelta blosc_testing)

    # tests
    add_test(NAME test_plugin_bytedelta
        COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:test_bytedelta>)

#    # Copy test files
#    file(GLOB TESTS_DATA ../../test_data/*.b2nd)
#    foreach (data ${TESTS_DATA})
#        file(COPY ${data} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
#    endforeach(data)

endif()
