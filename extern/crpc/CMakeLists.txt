cmake_minimum_required(VERSION 3.8)

project(cRPC)

set(base
    src/context.c
    src/evloop.c
    src/hash_table.c
    src/net.c
    src/serial.c
    src/server.c
    src/client.c
)

set(jrpc_extend
    src/jrpc_server.c
    src/jrpc_client.c
    src/cJSON.c
)

set(pub_sub_extend
    src/pubsub_server.c
    src/map.c
    src/pubsub_client.c
)


add_definitions(-DUSE_SELECT -D__LINUX__)

add_subdirectory(extern/onemessage/src)

add_library(crpc_base SHARED ${base} ${jrpc_extend} ${pub_sub_extend})
target_include_directories(crpc_base PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/extern/onemessage/src
    ${PROJECT_SOURCE_DIR}/extern/onemessage/src/core
    ${PROJECT_SOURCE_DIR}/extern/onemessage/src/comp
    ${PROJECT_SOURCE_DIR}/extern/onemessage/src/app
)
target_link_libraries(crpc_base pthread OneMessage)
target_compile_definitions(crpc_base PUBLIC WITH_PUB_SUB)

add_subdirectory(test)
