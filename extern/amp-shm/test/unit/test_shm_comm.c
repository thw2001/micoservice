/**
 * @file test_shm_comm.c
 * @brief 共享内存通信模块单元测试
 * @copyright 2023 AMP-SHM Project, MIT License
 * 
 * 使用CuTest框架对shm_comm模块进行单元测试，覆盖主要功能点。
 */

#include "CuTest.h"
#include "shm_comm.h"
#include "shm_errors.h"
#include "shm_log.h"
#include <stdlib.h>
#include <string.h>

// 测试用的共享内存大小
#define TEST_SHM_SIZE (1024 * 1024)  // 1MB
#define TEST_DATA_SIZE 256
#define TEST_QUEUE_SIZE 10

// 全局测试变量
static void* g_test_shm_addr = NULL;
static shm_comm_config_t* g_test_config = NULL;
static shm_endpoint_table_t* g_test_endpoint_table = NULL;

/**
 * @brief 测试初始化函数
 * 
 * 在每个测试用例执行前调用，初始化测试环境。
 */
static void test_setup(CuTest* tc) {
    // 分配测试用的共享内存
    g_test_shm_addr = malloc(TEST_SHM_SIZE);
    CuAssertPtrNotNull(tc, g_test_shm_addr);
    
    // 清零内存
    memset(g_test_shm_addr, 0, TEST_SHM_SIZE);
    
    // 分配测试配置
    g_test_config = (shm_comm_config_t*)malloc(sizeof(shm_comm_config_t));
    CuAssertPtrNotNull(tc, g_test_config);
    memset(g_test_config, 0, sizeof(shm_comm_config_t));
    
    // 初始化日志级别，避免测试输出过多日志
}

/**
 * @brief 测试清理函数
 * 
 * 在每个测试用例执行后调用，清理测试环境。
 */
static void test_teardown(CuTest* tc) {
    if (g_test_config) {
        shm_comm_deinit(g_test_config);
        free(g_test_config);
        g_test_config = NULL;
    }
    if (g_test_shm_addr) {
        free(g_test_shm_addr);
        g_test_shm_addr = NULL;
    }
    g_test_endpoint_table = NULL;
}

/**
 * @brief 测试shm_comm_init_main函数 - 正常情况
 */
static void test_shm_comm_init_main_normal(CuTest* tc) {
    test_setup(tc);
    
    // 执行测试
    int result = shm_comm_init_main(g_test_config, g_test_shm_addr, TEST_SHM_SIZE);
    
    // 验证结果
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 验证端点表是否正确初始化
    g_test_endpoint_table = shm_get_endpoint_table(g_test_config);
    CuAssertPtrNotNull(tc, g_test_endpoint_table);
    CuAssertIntEquals(tc, TEST_SHM_SIZE, g_test_endpoint_table->size);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_comm_init_main函数 - 无效参数
 */
static void test_shm_comm_init_main_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 测试NULL配置
    int result = shm_comm_init_main(NULL, g_test_shm_addr, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试NULL地址
    result = shm_comm_init_main(g_test_config, NULL, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_comm_init_minor函数 - 正常情况
 */
static void test_shm_comm_init_minor_normal(CuTest* tc) {
    test_setup(tc);
    
    // 首先初始化主进程
    int result = shm_comm_init_main(g_test_config, g_test_shm_addr, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 然后初始化从进程
    shm_comm_config_t minor_config;
    memset(&minor_config, 0, sizeof(minor_config));
    result = shm_comm_init_minor(&minor_config, g_test_shm_addr);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_comm_init_minor函数 - 无效参数
 */
static void test_shm_comm_init_minor_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 测试NULL配置
    int result = shm_comm_init_minor(NULL, g_test_shm_addr);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试NULL地址
    result = shm_comm_init_minor(g_test_config, NULL);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_comm_config_add_channel函数 - 正常情况
 */
static void test_shm_comm_config_add_channel_normal(CuTest* tc) {
    test_setup(tc);
    
    // 执行测试
    int result = shm_comm_config_add_channel(g_test_config, "sender1", "receiver1", 
                                           TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    
    // 验证结果
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_comm_config_add_channel函数 - 无效参数
 */
static void test_shm_comm_config_add_channel_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 测试NULL配置
    int result = shm_comm_config_add_channel(NULL, "sender1", "receiver1", 
                                           TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试NULL发送者名称
    result = shm_comm_config_add_channel(g_test_config, NULL, "receiver1", 
                                       TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试NULL接收者名称
    result = shm_comm_config_add_channel(g_test_config, "sender1", NULL, 
                                       TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_comm_cal_mem函数
 */
static void test_shm_comm_cal_mem(CuTest* tc) {
    test_setup(tc);
    
    // 添加一些通道配置
    shm_comm_config_add_channel(g_test_config, "sender1", "receiver1", TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    shm_comm_config_add_channel(g_test_config, "sender2", "receiver2", TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    
    // 计算内存需求
    int mem_size = shm_comm_cal_mem(g_test_config);
    
    // 验证结果应该大于端点表大小
    CuAssertTrue(tc, mem_size > (int)sizeof(shm_endpoint_table_t));
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_comm_load_config函数 - 正常情况
 */
static void test_shm_comm_load_config_normal(CuTest* tc) {
    test_setup(tc);
    
    // 初始化主进程
    int result = shm_comm_init_main(g_test_config, g_test_shm_addr, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 添加通道配置
    result = shm_comm_config_add_channel(g_test_config, "sender1", "receiver1", 
                                       TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 加载配置
    result = shm_comm_load_config(g_test_config);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_comm_load_config函数 - 端点表未初始化
 */
static void test_shm_comm_load_config_not_initialized(CuTest* tc) {
    test_setup(tc);
    
    // 不初始化端点表，直接尝试加载配置
    int result = shm_comm_load_config(g_test_config);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_comm_clear函数
 */
static void test_shm_comm_clear(CuTest* tc) {
    test_setup(tc);
    
    // 初始化主进程
    int result = shm_comm_init_main(g_test_config, g_test_shm_addr, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 添加通道配置并加载
    result = shm_comm_config_add_channel(g_test_config, "sender1", "receiver1", 
                                       TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    result = shm_comm_load_config(g_test_config);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 清除配置
    result = shm_comm_clear(g_test_config);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 验证端点表已被清空
    g_test_endpoint_table = shm_get_endpoint_table(g_test_config);
    CuAssertIntEquals(tc, 0, g_test_endpoint_table->count);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_comm_create_endpoint_base函数 - 正常情况
 */
static void test_shm_comm_create_endpoint_base_normal(CuTest* tc) {
    test_setup(tc);
    
    // 初始化主进程
    int result = shm_comm_init_main(g_test_config, g_test_shm_addr, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 创建端点
    result = shm_comm_create_endpoint_base(g_test_config, "test_endpoint", TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_comm_create_endpoint_base函数 - 无效参数
 */
static void test_shm_comm_create_endpoint_base_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 初始化主进程
    int result = shm_comm_init_main(g_test_config, g_test_shm_addr, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 测试NULL配置
    result = shm_comm_create_endpoint_base(NULL, "test_endpoint", TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试NULL名称
    result = shm_comm_create_endpoint_base(g_test_config, NULL, TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_comm_get_channel函数 - 正常情况
 */
static void test_shm_comm_get_channel_normal(CuTest* tc) {
    test_setup(tc);
    
    // 初始化主进程
    int result = shm_comm_init_main(g_test_config, g_test_shm_addr, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 创建端点
    result = shm_comm_create_endpoint_base(g_test_config, "sender1->receiver1", TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 获取通道
    shm_endpoint_t* endpoint = shm_comm_get_channel(g_test_config, "sender1", "receiver1");
    CuAssertPtrNotNull(tc, endpoint);
    CuAssertStrEquals(tc, "sender1->receiver1", endpoint->name);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_comm_get_channel函数 - 无效参数
 */
static void test_shm_comm_get_channel_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 初始化主进程
    int result = shm_comm_init_main(g_test_config, g_test_shm_addr, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 测试NULL配置
    shm_endpoint_t* endpoint = shm_comm_get_channel(NULL, "sender1", "receiver1");
    CuAssertPtrEquals(tc, NULL, endpoint);
    
    // 测试NULL发送者
    endpoint = shm_comm_get_channel(g_test_config, NULL, "receiver1");
    CuAssertPtrEquals(tc, NULL, endpoint);
    
    // 测试NULL接收者
    endpoint = shm_comm_get_channel(g_test_config, "sender1", NULL);
    CuAssertPtrEquals(tc, NULL, endpoint);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_comm_send和shm_comm_recv函数 - 正常情况
 */
static void test_shm_comm_send_recv_normal(CuTest* tc) {
    test_setup(tc);
    
    // 初始化主进程
    int result = shm_comm_init_main(g_test_config, g_test_shm_addr, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 创建端点
    result = shm_comm_create_endpoint_base(g_test_config, "sender1->receiver1", TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 获取通道
    shm_endpoint_t* endpoint = shm_comm_get_channel(g_test_config, "sender1", "receiver1");
    CuAssertPtrNotNull(tc, endpoint);
    
    // 获取端点表
    shm_endpoint_table_t* ep_table = shm_get_endpoint_table(g_test_config);
    CuAssertPtrNotNull(tc, ep_table);
    
    // 准备测试数据
    char send_data[TEST_DATA_SIZE];
    char recv_data[TEST_DATA_SIZE];
    memset(send_data, 0xAA, TEST_DATA_SIZE);
    memset(recv_data, 0, TEST_DATA_SIZE);
    
    // 发送数据
    result = shm_comm_send(ep_table, endpoint, send_data, TEST_DATA_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 接收数据
    result = shm_comm_recv(ep_table, endpoint, recv_data, TEST_DATA_SIZE);
    CuAssertIntEquals(tc, TEST_DATA_SIZE, result);
    
    // 验证数据
    CuAssertIntEquals(tc, 0, memcmp(send_data, recv_data, TEST_DATA_SIZE));
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_comm_send函数 - 无效参数
 */
static void test_shm_comm_send_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 初始化主进程
    int result = shm_comm_init_main(g_test_config, g_test_shm_addr, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 创建端点
    result = shm_comm_create_endpoint_base(g_test_config, "sender1->receiver1", TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 获取通道
    shm_endpoint_t* endpoint = shm_comm_get_channel(g_test_config, "sender1", "receiver1");
    CuAssertPtrNotNull(tc, endpoint);
    
    // 获取端点表
    shm_endpoint_table_t* ep_table = shm_get_endpoint_table(g_test_config);
    CuAssertPtrNotNull(tc, ep_table);
    
    // 测试NULL端点表
    result = shm_comm_send(NULL, endpoint, "test", 4);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试NULL端点
    result = shm_comm_send(ep_table, NULL, "test", 4);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试NULL数据
    result = shm_comm_send(ep_table, endpoint, NULL, 4);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_comm_recv函数 - 无效参数
 */
static void test_shm_comm_recv_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 初始化主进程
    int result = shm_comm_init_main(g_test_config, g_test_shm_addr, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 创建端点
    result = shm_comm_create_endpoint_base(g_test_config, "sender1->receiver1", TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 获取通道
    shm_endpoint_t* endpoint = shm_comm_get_channel(g_test_config, "sender1", "receiver1");
    CuAssertPtrNotNull(tc, endpoint);
    
    // 获取端点表
    shm_endpoint_table_t* ep_table = shm_get_endpoint_table(g_test_config);
    CuAssertPtrNotNull(tc, ep_table);
    
    // 测试NULL端点表
    char buffer[TEST_DATA_SIZE];
    result = shm_comm_recv(NULL, endpoint, buffer, TEST_DATA_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试NULL端点
    result = shm_comm_recv(ep_table, NULL, buffer, TEST_DATA_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试NULL缓冲区
    result = shm_comm_recv(ep_table, endpoint, NULL, TEST_DATA_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试零大小缓冲区
    result = shm_comm_recv(ep_table, endpoint, buffer, 0);
    CuAssertIntEquals(tc, SHM_COMM_BUFFER_TOO_SMALL, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试零拷贝发送接收函数
 */
static void test_shm_comm_zero_copy_send_recv(CuTest* tc) {
    test_setup(tc);
    
    // 初始化主进程
    int result = shm_comm_init_main(g_test_config, g_test_shm_addr, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 创建端点
    result = shm_comm_create_endpoint_base(g_test_config, "sender1->receiver1", TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 获取通道
    shm_endpoint_t* endpoint = shm_comm_get_channel(g_test_config, "sender1", "receiver1");
    CuAssertPtrNotNull(tc, endpoint);
    
    // 获取端点表
    shm_endpoint_table_t* ep_table = shm_get_endpoint_table(g_test_config);
    CuAssertPtrNotNull(tc, ep_table);
    
    // 零拷贝发送
    shm_base_msg_t* send_msg = shm_comm_send_alloc(ep_table, endpoint, TEST_DATA_SIZE);
    CuAssertPtrNotNull(tc, send_msg);
    
    // 填充数据
    memset(send_msg->payload, 0xBB, TEST_DATA_SIZE);
    
    // 完成发送
    shm_comm_send_alloc_end(ep_table, endpoint);
    
    // 零拷贝接收
    shm_base_msg_t* recv_msg = shm_comm_recv_zc(ep_table, endpoint);
    CuAssertPtrNotNull(tc, recv_msg);
    
    // 验证数据
    CuAssertIntEquals(tc, 0, memcmp(send_msg->payload, recv_msg->payload, TEST_DATA_SIZE));
    
    // 完成接收
    shm_comm_recv_zc_end(endpoint);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_comm_send_alloc函数 - 无效参数
 */
static void test_shm_comm_send_alloc_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 初始化主进程
    int result = shm_comm_init_main(g_test_config, g_test_shm_addr, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 创建端点
    result = shm_comm_create_endpoint_base(g_test_config, "sender1->receiver1", TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 获取通道
    shm_endpoint_t* endpoint = shm_comm_get_channel(g_test_config, "sender1", "receiver1");
    CuAssertPtrNotNull(tc, endpoint);
    
    // 获取端点表
    shm_endpoint_table_t* ep_table = shm_get_endpoint_table(g_test_config);
    CuAssertPtrNotNull(tc, ep_table);
    
    // 测试NULL端点表
    shm_base_msg_t* msg = shm_comm_send_alloc(NULL, endpoint, TEST_DATA_SIZE);
    CuAssertPtrEquals(tc, NULL, msg);
    
    // 测试NULL端点
    msg = shm_comm_send_alloc(ep_table, NULL, TEST_DATA_SIZE);
    CuAssertPtrEquals(tc, NULL, msg);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_comm_recv_zc函数 - 无效参数
 */
static void test_shm_comm_recv_zc_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 初始化主进程
    int result = shm_comm_init_main(g_test_config, g_test_shm_addr, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 创建端点
    result = shm_comm_create_endpoint_base(g_test_config, "sender1->receiver1", TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 获取通道
    shm_endpoint_t* endpoint = shm_comm_get_channel(g_test_config, "sender1", "receiver1");
    CuAssertPtrNotNull(tc, endpoint);
    
    // 获取端点表
    shm_endpoint_table_t* ep_table = shm_get_endpoint_table(g_test_config);
    CuAssertPtrNotNull(tc, ep_table);
    
    // 测试NULL端点表
    shm_base_msg_t* msg = shm_comm_recv_zc(NULL, endpoint);
    CuAssertPtrEquals(tc, NULL, msg);
    
    // 测试NULL端点
    msg = shm_comm_recv_zc(ep_table, NULL);
    CuAssertPtrEquals(tc, NULL, msg);
    
    test_teardown(tc);
}

/**
 * @brief 测试队列满的情况
 */
static void test_shm_comm_queue_full(CuTest* tc) {
    test_setup(tc);
    
    // 初始化主进程
    int result = shm_comm_init_main(g_test_config, g_test_shm_addr, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 创建小队列（容量为1），因为内部为环形队列，所以消息容量是队列大小-1
    result = shm_comm_create_endpoint_base(g_test_config, "sender1->receiver1", TEST_DATA_SIZE, 2);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 获取通道
    shm_endpoint_t* endpoint = shm_comm_get_channel(g_test_config, "sender1", "receiver1");
    CuAssertPtrNotNull(tc, endpoint);
    
    // 获取端点表
    shm_endpoint_table_t* ep_table = shm_get_endpoint_table(g_test_config);
    CuAssertPtrNotNull(tc, ep_table);
    
    // 准备测试数据
    char send_data[TEST_DATA_SIZE];
    memset(send_data, 0xAA, TEST_DATA_SIZE);
    
    // 发送第一个消息（应该成功）
    result = shm_comm_send(ep_table, endpoint, send_data, TEST_DATA_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 发送第二个消息（应该失败，队列已满）
    result = shm_comm_send(ep_table, endpoint, send_data, TEST_DATA_SIZE);
    CuAssertIntEquals(tc, SHM_QUEUE_FULL, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试队列空的情况
 */
static void test_shm_comm_queue_empty(CuTest* tc) {
    test_setup(tc);
    
    // 初始化主进程
    int result = shm_comm_init_main(g_test_config, g_test_shm_addr, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 创建端点
    result = shm_comm_create_endpoint_base(g_test_config, "sender1->receiver1", TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 获取通道
    shm_endpoint_t* endpoint = shm_comm_get_channel(g_test_config, "sender1", "receiver1");
    CuAssertPtrNotNull(tc, endpoint);
    
    // 获取端点表
    shm_endpoint_table_t* ep_table = shm_get_endpoint_table(g_test_config);
    CuAssertPtrNotNull(tc, ep_table);
    
    // 尝试从空队列接收
    char recv_data[TEST_DATA_SIZE];
    result = shm_comm_recv(ep_table, endpoint, recv_data, TEST_DATA_SIZE);
    CuAssertIntEquals(tc, SHM_QUEUE_EMPTY, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试统计信息更新
 */
static void test_shm_comm_stats_update(CuTest* tc) {
    test_setup(tc);
    
    // 初始化主进程
    int result = shm_comm_init_main(g_test_config, g_test_shm_addr, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 创建端点
    result = shm_comm_create_endpoint_base(g_test_config, "sender1->receiver1", TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 获取通道
    shm_endpoint_t* endpoint = shm_comm_get_channel(g_test_config, "sender1", "receiver1");
    CuAssertPtrNotNull(tc, endpoint);
    
    // 获取端点表
    shm_endpoint_table_t* ep_table = shm_get_endpoint_table(g_test_config);
    CuAssertPtrNotNull(tc, ep_table);
    
    // 记录初始统计信息
    uint64_t initial_bytes_sent = endpoint->bytes_sent;
    uint64_t initial_bytes_received = endpoint->bytes_received;
    uint32_t initial_total_sends = endpoint->total_sends;
    uint32_t initial_total_receives = endpoint->total_receives;
    
    // 准备测试数据
    char send_data[TEST_DATA_SIZE];
    char recv_data[TEST_DATA_SIZE];
    memset(send_data, 0xAA, TEST_DATA_SIZE);
    memset(recv_data, 0, TEST_DATA_SIZE);
    
    // 发送数据
    result = shm_comm_send(ep_table, endpoint, send_data, TEST_DATA_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 验证发送统计信息已更新
    CuAssertTrue(tc, endpoint->bytes_sent > initial_bytes_sent);
    CuAssertTrue(tc, endpoint->total_sends > initial_total_sends);
    
    // 接收数据
    result = shm_comm_recv(ep_table, endpoint, recv_data, TEST_DATA_SIZE);
    CuAssertIntEquals(tc, TEST_DATA_SIZE, result);
    
    // 验证接收统计信息已更新
    CuAssertTrue(tc, endpoint->bytes_received > initial_bytes_received);
    CuAssertTrue(tc, endpoint->total_receives > initial_total_receives);
    
    test_teardown(tc);
}

/**
 * @brief 获取shm_comm模块的测试套件
 * 
 * @return CuSuite* 测试套件指针
 */
CuSuite* ShmCommGetSuite() {
    CuSuite* suite = CuSuiteNew();
    
    // 初始化相关测试
    SUITE_ADD_TEST(suite, test_shm_comm_init_main_normal);
    SUITE_ADD_TEST(suite, test_shm_comm_init_main_invalid_param);
    SUITE_ADD_TEST(suite, test_shm_comm_init_minor_normal);
    SUITE_ADD_TEST(suite, test_shm_comm_init_minor_invalid_param);
    
    // 配置相关测试
    SUITE_ADD_TEST(suite, test_shm_comm_config_add_channel_normal);
    SUITE_ADD_TEST(suite, test_shm_comm_config_add_channel_invalid_param);
    SUITE_ADD_TEST(suite, test_shm_comm_cal_mem);
    SUITE_ADD_TEST(suite, test_shm_comm_load_config_normal);
    SUITE_ADD_TEST(suite, test_shm_comm_load_config_not_initialized);
    SUITE_ADD_TEST(suite, test_shm_comm_clear);
    
    // 端点管理相关测试
    SUITE_ADD_TEST(suite, test_shm_comm_create_endpoint_base_normal);
    SUITE_ADD_TEST(suite, test_shm_comm_create_endpoint_base_invalid_param);
    SUITE_ADD_TEST(suite, test_shm_comm_get_channel_normal);
    SUITE_ADD_TEST(suite, test_shm_comm_get_channel_invalid_param);
    
    // 消息发送接收相关测试
    SUITE_ADD_TEST(suite, test_shm_comm_send_recv_normal);
    SUITE_ADD_TEST(suite, test_shm_comm_send_invalid_param);
    SUITE_ADD_TEST(suite, test_shm_comm_recv_invalid_param);
    SUITE_ADD_TEST(suite, test_shm_comm_zero_copy_send_recv);
    SUITE_ADD_TEST(suite, test_shm_comm_send_alloc_invalid_param);
    SUITE_ADD_TEST(suite, test_shm_comm_recv_zc_invalid_param);
    
    // 边界条件测试
    SUITE_ADD_TEST(suite, test_shm_comm_queue_full);
    SUITE_ADD_TEST(suite, test_shm_comm_queue_empty);
    
    // 统计信息测试
    SUITE_ADD_TEST(suite, test_shm_comm_stats_update);
    
    return suite;
}
