cmake_minimum_required(VERSION 3.10)

# set(CMAKE_C_STANDARD 89)

# Find all C files in the directory
file(GLOB SOURCES "*.c" "*.h")

# Add cJSON library as a dynamic library
add_library(base SHARED ${SOURCES})

target_include_directories(base PUBLIC
    ${CMAKE_SOURCE_DIR}/util
)

target_link_libraries(base PUBLIC
    ms_util
)