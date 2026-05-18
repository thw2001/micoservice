project(MSF LANGUAGES C)

cmake_minimum_required(VERSION 3.10)

# MYLIB_PATH作为编译成果的输出，用于特定工具链指定输出
if(NOT DEFINED MYLIB_PATH)
    set(MYLIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/output")
    file(MAKE_DIRECTORY ${MYLIB_PATH})

    set(LIBRARY_OUTPUT_PATH ${MYLIB_PATH})
    set(EXECUTABLE_OUTPUT_PATH ${MYLIB_PATH})
endif()

add_definitions(-DUSE_SELECT -D__LINUX__ -DWITH_PUB_SUB -DWITH_HTTP_SERVER -D__SHM_USE_SHMGET__ -DFS_BASE="")

# add_subdirectory(extern/crpc)
add_subdirectory(extern/amp-shm)
# add_subdirectory(extern/amp-shm/src)
# add_subdirectory(extern/amp-shm/message)

add_subdirectory(util)
add_subdirectory(comm)
add_subdirectory(base)
add_subdirectory(mstracing)
add_subdirectory(micoservice)
add_subdirectory(node)
add_subdirectory(registry)
add_subdirectory(test)
