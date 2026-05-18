# Unit tests CMakeLists.txt
# 共享内存通信模块单元测试构建配置

# 设置C标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/cutest
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/message
)

# CuTest框架源文件
set(CUTEST_SOURCES
    cutest/CuTest.c
    cutest/CuTestTest.c
)

# 测试源文件
set(TEST_SOURCES
    test_shm_comm.c
    test_shm_queue.c
    test_shm_endpoint.c
    test_pubsub.c
    test_json_config_processor.c
    cutest/AllTests.c
)

# 创建单元测试可执行文件
add_executable(unit_tests ${TEST_SOURCES} ${CUTEST_SOURCES})

# 链接依赖库
target_link_libraries(unit_tests
    PRIVATE
    xnet_shm_lib
    om
    pthread
)

# 设置测试属性
# set_target_properties(unit_tests PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
# )

# 添加编译定义
target_compile_definitions(unit_tests
    PRIVATE
    -D__LINUX__
    -DSHM_COMM
    -D__RTP__
)

# 添加测试到CTest（如果启用测试）
# enable_testing()
# add_test(NAME unit_tests COMMAND unit_tests)
