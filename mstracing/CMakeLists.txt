cmake_minimum_required(VERSION 3.10)

# set(CMAKE_C_STANDARD 89)

# Find all C files in the directory
file(GLOB SOURCES "*.c" "*.h")

add_library(mstracing SHARED ${SOURCES})

target_include_directories(mstracing PUBLIC
    ${CMAKE_SOURCE_DIR}/base
    ${CMAKE_SOURCE_DIR}/comm
)

target_compile_definitions(mstracing PUBLIC

)

target_link_libraries(mstracing PUBLIC
    base
    ms_comm
)
