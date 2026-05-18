cmake_minimum_required(VERSION 3.10)
project(xnet_shm LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_definitions(-D__LINUX__ -DSHM_COMM -D__RTP__ -fPIC)

# Build options
option(BUILD_EXAMPLES "Build example programs" ON)
option(BUILD_TESTS "Build unit tests" ON)

# Print build configuration
message(STATUS "Build configuration:")
message(STATUS "  BUILD_EXAMPLES: ${BUILD_EXAMPLES}")
message(STATUS "  BUILD_TESTS:    ${BUILD_TESTS}")

# Add src subdirectory (always built)
add_subdirectory(message)
add_subdirectory(src)

# Add example subdirectory conditionally
if(BUILD_EXAMPLES)
    add_subdirectory(example)
else()
    message(STATUS "Examples disabled - skipping example directory")
endif()

# Add test subdirectory conditionally
if(BUILD_TESTS)
    add_subdirectory(test/unit)
else()
    message(STATUS "Tests disabled - skipping test directory")
endif()
