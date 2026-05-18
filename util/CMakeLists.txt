cmake_minimum_required(VERSION 3.10)

# set(CMAKE_C_STANDARD 89)

# Find all C files in the directory
file(GLOB SOURCES "*.c" "*.h")

add_library(ms_util SHARED ${SOURCES})

