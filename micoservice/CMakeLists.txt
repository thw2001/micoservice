cmake_minimum_required(VERSION 3.10)

# set(CMAKE_C_STANDARD 89)

# Find all C files in the directory
file(GLOB SOURCES "*.c" "*.h")

add_library(micoserice SHARED ${SOURCES})

target_include_directories(micoserice PUBLIC
    ${CMAKE_SOURCE_DIR}/base
    ${CMAKE_SOURCE_DIR}/comm
)

target_link_libraries(micoserice PUBLIC
    base
    ms_comm
)
