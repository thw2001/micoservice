cmake_minimum_required(VERSION 3.10)

# set(CMAKE_C_STANDARD 89)

# Find all C files in the directory
file(GLOB SOURCES "*.c" "*.h")

add_library(node_loader SHARED ${SOURCES})

target_include_directories(node_loader PUBLIC
    ${CMAKE_SOURCE_DIR}/base
    ${CMAKE_SOURCE_DIR}/comm
)

target_compile_definitions(node_loader PUBLIC

)

target_link_libraries(node_loader PUBLIC
    base
    ms_comm
)
