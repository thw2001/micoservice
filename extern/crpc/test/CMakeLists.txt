cmake_minimum_required(VERSION 3.8)



add_executable(client rpc_client.c)
target_include_directories(client PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(client crpc_base pthread)

add_executable(server rpc_server)
target_include_directories(server PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(server crpc_base pthread)

add_executable(json_rpc_server json_rpc_server.c)
target_include_directories(json_rpc_server PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(json_rpc_server crpc_base pthread)


add_executable(json_rpc_client json_rpc_client.c)
target_include_directories(json_rpc_client PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(json_rpc_client crpc_base pthread)

add_executable(pubsub_test_broker pubsub_test_broker.c)
target_include_directories(pubsub_test_broker PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(pubsub_test_broker crpc_base pthread OneMessage)

add_executable(pubsub_test_client pubsub_test_client.c)
target_include_directories(pubsub_test_client PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(pubsub_test_client crpc_base pthread OneMessage)
