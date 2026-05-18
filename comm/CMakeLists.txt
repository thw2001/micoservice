cmake_minimum_required(VERSION 3.10)

# set(CMAKE_C_STANDARD 89)

# Find all C files in the directory
# file(GLOB SOURCES "crpc/*.c" "*.h")
file(GLOB SOURCES "amp-shm/*.c" "*.h" "rpc.c" "rpc_example.c")

add_library(ms_comm SHARED ${SOURCES})

target_include_directories(ms_comm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/base
    ${CMAKE_SOURCE_DIR}/micoservice
    ${CMAKE_SOURCE_DIR}/util
    # ${CMAKE_SOURCE_DIR}/extern/crpc/include
    ${CMAKE_SOURCE_DIR}/extern/amp-shm/include
    ${CMAKE_SOURCE_DIR}/extern/amp-shm/message
    ${CMAKE_SOURCE_DIR}/extern/amp-shm/message/app
    ${CMAKE_SOURCE_DIR}/extern/amp-shm/message/comp
    ${CMAKE_SOURCE_DIR}/extern/amp-shm/message/core
)

target_compile_definitions(ms_comm PUBLIC
)

target_link_libraries(ms_comm PUBLIC
    om
    xnet_shm_lib
    pthread
    ms_util
    # crpc_base
)
