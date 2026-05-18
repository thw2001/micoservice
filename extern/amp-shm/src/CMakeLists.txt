# Source files
set(SRC_FILES
    cJSON.c
    pubsub.c
    shm_comm.c
    shm_endpoint.c 
    shm_queue.c
    vec.c
    json_config_processor.c
)

# Create library from source files
add_library(xnet_shm_lib ${SRC_FILES})

# Include directories
target_include_directories(xnet_shm_lib
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(xnet_shm_lib 
    PUBLIC
    om
)
