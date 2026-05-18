cmake_minimum_required(VERSION 3.10)

# set(CMAKE_C_STANDARD 89)

include_directories(
    ${CMAKE_SOURCE_DIR}/base
    ${CMAKE_SOURCE_DIR}/micoservice
    # ${CMAKE_SOURCE_DIR}/extern/crpc/include
    ${CMAKE_SOURCE_DIR}/registry
    ${CMAKE_SOURCE_DIR}/mstracing
    ${CMAKE_SOURCE_DIR}/node
    ${CMAKE_SOURCE_DIR}/extern/amp-shm/include
    ${CMAKE_SOURCE_DIR}/extern/amp-shm/message
    ${CMAKE_SOURCE_DIR}/extern/amp-shm/message/app
    ${CMAKE_SOURCE_DIR}/extern/amp-shm/message/comp
    ${CMAKE_SOURCE_DIR}/extern/amp-shm/message/core
)
link_libraries(
    base
    ms_comm
    micoserice
    registry
    mstracing
    node_loader
)

add_executable(micoserice_test micoservice_test.c)

add_executable(registry_i_test registry_i_test.c)

add_executable(mstracing_test1 mstracing_test1.c)
add_executable(mstracing_test2 mstracing_test2.c)
add_executable(tracing_test tracing_test.c)
add_executable(lb_recv lb_recv.c)
add_executable(lb_send lb_send.c)
add_executable(node_loader_test node_loader_test.c)

add_executable(shm_registry shm_registry.c)
add_executable(shm_amp_config shm_amp_config.c)
add_executable(test1 test1.c)
add_executable(test2 test2.c)