/**
 * @file test_shm_endpoint.c
 * @brief 共享内存端点管理模块单元测试
 * @copyright 2023 AMP-SHM Project, MIT License
 * 
 * 使用CuTest框架对shm_endpoint模块进行单元测试，覆盖主要功能点。
 * 遵循C语言工程化开发规范和TDD最佳实践。
 */

#include "CuTest.h"
#include "shm_endpoint.h"
#include "shm_errors.h"
#include "shm_log.h"
#include <stdlib.h>
#include <string.h>

// 测试用的共享内存大小
#define TEST_SHM_SIZE (1024 * 1024)  // 1MB
#define TEST_DATA_SIZE 256
#define TEST_QUEUE_SIZE 10
#define TEST_ENDPOINT_NAME "test_endpoint"
#define TEST_ENDPOINT_NAME2 "test_endpoint2"

// 全局测试变量
static void* g_test_shm_addr = NULL;
static shm_endpoint_table_t g_test_table;

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
    
    // 初始化日志级别，避免测试输出过多日志
    
    
    // 初始化测试端点表
    memset(&g_test_table, 0, sizeof(shm_endpoint_table_t));
    
    int result = shm_endpoint_table_init(&g_test_table, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
}

/**
 * @brief 测试清理函数
 * 
 * 在每个测试用例执行后调用，清理测试环境。
 */
static void test_teardown(CuTest* tc) {
    if (g_test_shm_addr) {
        free(g_test_shm_addr);
        g_test_shm_addr = NULL;
    }
    memset(&g_test_table, 0, sizeof(shm_endpoint_table_t));
}

/**
 * @brief 测试shm_endpoint_table_init函数 - 正常情况
 */
static void test_shm_endpoint_table_init_normal(CuTest* tc) {
    test_setup(tc);
    
    // 重新初始化验证
    shm_endpoint_table_t table;
    memset(&table, 0, sizeof(shm_endpoint_table_t));
    table.size = TEST_SHM_SIZE;
    
    int result = shm_endpoint_table_init(&table, TEST_SHM_SIZE);
    
    // 验证结果
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    CuAssertIntEquals(tc, 0, table.count);
    CuAssertIntEquals(tc, sizeof(shm_endpoint_table_t), table.offset_in_shm);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_endpoint_table_init函数 - 无效参数
 */
static void test_shm_endpoint_table_init_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 测试NULL表指针
    int result = shm_endpoint_table_init(NULL, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_endpoint_table_init函数 - size太小
 */
static void test_shm_endpoint_table_init_size_too_small(CuTest* tc) {
    test_setup(tc);
    
    shm_endpoint_table_t table;
    memset(&table, 0, sizeof(shm_endpoint_table_t));
    
    // 测试size小于结构体大小
    uint32_t small_size = sizeof(shm_endpoint_table_t) - 1;
    int result = shm_endpoint_table_init(&table, small_size);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试size等于结构体大小（应该成功）
    uint32_t exact_size = sizeof(shm_endpoint_table_t);
    result = shm_endpoint_table_init(&table, exact_size);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    CuAssertIntEquals(tc, exact_size, table.size);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_endpoint_register_base函数 - 正常情况
 */
static void test_shm_endpoint_register_base_normal(CuTest* tc) {
    test_setup(tc);
    
    // 执行测试
    int result = shm_endpoint_register_base(&g_test_table, TEST_ENDPOINT_NAME, 
                                          TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    
    // 验证结果
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    CuAssertIntEquals(tc, 1, g_test_table.count);
    
    // 验证端点是否正确注册
    shm_endpoint_t* endpoint = shm_endpoint_find(&g_test_table, TEST_ENDPOINT_NAME);
    CuAssertPtrNotNull(tc, endpoint);
    CuAssertStrEquals(tc, TEST_ENDPOINT_NAME, endpoint->name);
    CuAssertIntEquals(tc, TEST_DATA_SIZE, endpoint->queue.data_size);
    CuAssertIntEquals(tc, TEST_QUEUE_SIZE, endpoint->queue.size);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_endpoint_register_base函数 - 无效参数
 */
static void test_shm_endpoint_register_base_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 测试NULL表指针
    int result = shm_endpoint_register_base(NULL, TEST_ENDPOINT_NAME, 
                                          TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试NULL名称
    result = shm_endpoint_register_base(&g_test_table, NULL, 
                                      TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试零数据大小
    result = shm_endpoint_register_base(&g_test_table, TEST_ENDPOINT_NAME, 
                                      0, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_QUEUE_INVALID_SIZE, result);
    
    // 测试零队列大小
    result = shm_endpoint_register_base(&g_test_table, TEST_ENDPOINT_NAME, 
                                      TEST_DATA_SIZE, 0);
    CuAssertIntEquals(tc, SHM_QUEUE_INVALID_SIZE, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试端点名称验证 - 空名称
 */
static void test_shm_endpoint_register_base_empty_name(CuTest* tc) {
    test_setup(tc);
    
    // 测试空名称
    int result = shm_endpoint_register_base(&g_test_table, "", 
                                          TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试端点名称验证 - 过长名称
 */
static void test_shm_endpoint_register_base_long_name(CuTest* tc) {
    test_setup(tc);
    
    // 创建过长的名称
    char long_name[MAX_ENDPOINT_NAME + 10];
    memset(long_name, 'A', sizeof(long_name));
    long_name[sizeof(long_name) - 1] = '\0';
    
    int result = shm_endpoint_register_base(&g_test_table, long_name, 
                                          TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试端点名称验证 - 边界长度名称
 */
static void test_shm_endpoint_register_base_boundary_name(CuTest* tc) {
    test_setup(tc);
    
    // 创建最大长度的名称
    char boundary_name[MAX_ENDPOINT_NAME];
    memset(boundary_name, 'B', sizeof(boundary_name) - 1);
    boundary_name[sizeof(boundary_name) - 1] = '\0';
    
    int result = shm_endpoint_register_base(&g_test_table, boundary_name, 
                                          TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试重复端点注册
 */
static void test_shm_endpoint_register_base_duplicate(CuTest* tc) {
    test_setup(tc);
    
    // 第一次注册
    int result = shm_endpoint_register_base(&g_test_table, TEST_ENDPOINT_NAME, 
                                          TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 尝试重复注册
    result = shm_endpoint_register_base(&g_test_table, TEST_ENDPOINT_NAME, 
                                      TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_ENDPOINT_EXISTS, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试端点表已满情况
 */
static void test_shm_endpoint_register_base_table_full(CuTest* tc) {
    test_setup(tc);
    
    char name[32];
    
    // 填满端点表
    for (int i = 0; i < MAX_ENDPOINTS; i++) {
        snprintf(name, sizeof(name), "endpoint_%d", i);
        int result = shm_endpoint_register_base(&g_test_table, name, 
                                              TEST_DATA_SIZE, TEST_QUEUE_SIZE);
        CuAssertIntEquals(tc, SHM_SUCCESS, result);
    }
    
    // 尝试注册第MAX_ENDPOINTS+1个端点
    int result = shm_endpoint_register_base(&g_test_table, "extra_endpoint", 
                                          TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_ENDPOINT_TABLE_FULL, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试共享内存不足情况
 */
static void test_shm_endpoint_register_base_insufficient_memory(CuTest* tc) {
    test_setup(tc);
    
    // 设置一个很小的共享内存大小，不足以容纳端点
    g_test_table.size = sizeof(shm_endpoint_table_t) + 100;  // 只有100字节额外空间
    
    // 尝试注册需要更大空间的端点
    int result = shm_endpoint_register_base(&g_test_table, TEST_ENDPOINT_NAME, 
                                          TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_MEMORY, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_endpoint_unregister函数 - 正常情况
 */
static void test_shm_endpoint_unregister_normal(CuTest* tc) {
    test_setup(tc);
    
    // 先注册一个端点
    int result = shm_endpoint_register_base(&g_test_table, TEST_ENDPOINT_NAME, 
                                          TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 注销端点
    result = shm_endpoint_unregister(&g_test_table, TEST_ENDPOINT_NAME);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    CuAssertIntEquals(tc, 0, g_test_table.count);
    
    // 验证端点确实被删除
    shm_endpoint_t* endpoint = shm_endpoint_find(&g_test_table, TEST_ENDPOINT_NAME);
    CuAssertPtrEquals(tc, NULL, endpoint);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_endpoint_unregister函数 - 无效参数
 */
static void test_shm_endpoint_unregister_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 测试NULL表指针
    int result = shm_endpoint_unregister(NULL, TEST_ENDPOINT_NAME);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试NULL名称
    result = shm_endpoint_unregister(&g_test_table, NULL);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试空名称
    result = shm_endpoint_unregister(&g_test_table, "");
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试注销不存在的端点
 */
static void test_shm_endpoint_unregister_not_found(CuTest* tc) {
    test_setup(tc);
    
    // 尝试注销不存在的端点
    int result = shm_endpoint_unregister(&g_test_table, "nonexistent_endpoint");
    CuAssertIntEquals(tc, SHM_ENDPOINT_NOT_FOUND, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试注销多个端点中的中间端点
 */
static void test_shm_endpoint_unregister_middle_endpoint(CuTest* tc) {
    test_setup(tc);
    
    // 注册多个端点
    int result = shm_endpoint_register_base(&g_test_table, "endpoint1", 
                                          TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    result = shm_endpoint_register_base(&g_test_table, "endpoint2", 
                                      TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    result = shm_endpoint_register_base(&g_test_table, "endpoint3", 
                                      TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    CuAssertIntEquals(tc, 3, g_test_table.count);
    
    // 注销中间的端点（endpoint2）
    result = shm_endpoint_unregister(&g_test_table, "endpoint2");
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    CuAssertIntEquals(tc, 2, g_test_table.count);
    
    // 验证剩余的端点
    shm_endpoint_t* endpoint1 = shm_endpoint_find(&g_test_table, "endpoint1");
    CuAssertPtrNotNull(tc, endpoint1);
    
    shm_endpoint_t* endpoint3 = shm_endpoint_find(&g_test_table, "endpoint3");
    CuAssertPtrNotNull(tc, endpoint3);
    
    shm_endpoint_t* endpoint2 = shm_endpoint_find(&g_test_table, "endpoint2");
    CuAssertPtrEquals(tc, NULL, endpoint2);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_endpoint_find函数 - 正常情况
 */
static void test_shm_endpoint_find_normal(CuTest* tc) {
    test_setup(tc);
    
    // 注册一个端点
    int result = shm_endpoint_register_base(&g_test_table, TEST_ENDPOINT_NAME, 
                                          TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 查找端点
    shm_endpoint_t* endpoint = shm_endpoint_find(&g_test_table, TEST_ENDPOINT_NAME);
    CuAssertPtrNotNull(tc, endpoint);
    CuAssertStrEquals(tc, TEST_ENDPOINT_NAME, endpoint->name);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_endpoint_find函数 - 无效参数
 */
static void test_shm_endpoint_find_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 测试NULL表指针
    shm_endpoint_t* endpoint = shm_endpoint_find(NULL, TEST_ENDPOINT_NAME);
    CuAssertPtrEquals(tc, NULL, endpoint);
    
    // 测试NULL名称
    endpoint = shm_endpoint_find(&g_test_table, NULL);
    CuAssertPtrEquals(tc, NULL, endpoint);
    
    // 测试空名称
    endpoint = shm_endpoint_find(&g_test_table, "");
    CuAssertPtrEquals(tc, NULL, endpoint);
    
    test_teardown(tc);
}

/**
 * @brief 测试查找不存在的端点
 */
static void test_shm_endpoint_find_not_found(CuTest* tc) {
    test_setup(tc);
    
    // 查找不存在的端点
    shm_endpoint_t* endpoint = shm_endpoint_find(&g_test_table, "nonexistent_endpoint");
    CuAssertPtrEquals(tc, NULL, endpoint);
    
    test_teardown(tc);
}

/**
 * @brief 测试查找多个具有相似名称的端点
 */
static void test_shm_endpoint_find_similar_names(CuTest* tc) {
    test_setup(tc);
    
    // 注册具有相似名称的端点
    int result = shm_endpoint_register_base(&g_test_table, "test_endpoint_1", 
                                          TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    result = shm_endpoint_register_base(&g_test_table, "test_endpoint_2", 
                                      TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    result = shm_endpoint_register_base(&g_test_table, "test_endpoint", 
                                      TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 精确查找
    shm_endpoint_t* endpoint = shm_endpoint_find(&g_test_table, "test_endpoint");
    CuAssertPtrNotNull(tc, endpoint);
    CuAssertStrEquals(tc, "test_endpoint", endpoint->name);
    
    endpoint = shm_endpoint_find(&g_test_table, "test_endpoint_1");
    CuAssertPtrNotNull(tc, endpoint);
    CuAssertStrEquals(tc, "test_endpoint_1", endpoint->name);
    
    endpoint = shm_endpoint_find(&g_test_table, "test_endpoint_2");
    CuAssertPtrNotNull(tc, endpoint);
    CuAssertStrEquals(tc, "test_endpoint_2", endpoint->name);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_endpoint_table_print函数 - 正常情况
 */
static void test_shm_endpoint_table_print_normal(CuTest* tc) {
    test_setup(tc);
    
    // 注册几个端点
    int result = shm_endpoint_register_base(&g_test_table, "endpoint1", 
                                          TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    result = shm_endpoint_register_base(&g_test_table, "endpoint2", 
                                      TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 调用打印函数（主要是验证不会崩溃）
    shm_endpoint_table_print(&g_test_table);
    
    // 验证函数执行后表状态不变
    CuAssertIntEquals(tc, 2, g_test_table.count);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_endpoint_table_print函数 - 无效参数
 */
static void test_shm_endpoint_table_print_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 测试NULL表指针（应该不会崩溃）
    shm_endpoint_table_print(NULL);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_endpoint_table_print函数 - 空表
 */
static void test_shm_endpoint_table_print_empty(CuTest* tc) {
    test_setup(tc);
    
    // 打印空表
    shm_endpoint_table_print(&g_test_table);
    
    // 验证表状态
    CuAssertIntEquals(tc, 0, g_test_table.count);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_endpoint_table_print_stats函数 - 正常情况
 */
static void test_shm_endpoint_table_print_stats_normal(CuTest* tc) {
    test_setup(tc);
    
    // 注册几个端点
    int result = shm_endpoint_register_base(&g_test_table, "endpoint1", 
                                          TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    result = shm_endpoint_register_base(&g_test_table, "endpoint2", 
                                      TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 调用统计打印函数（主要是验证不会崩溃）
    shm_endpoint_table_print_stats(&g_test_table);
    
    // 验证函数执行后表状态不变
    CuAssertIntEquals(tc, 2, g_test_table.count);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_endpoint_table_print_stats函数 - 无效参数
 */
static void test_shm_endpoint_table_print_stats_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 测试NULL表指针（应该不会崩溃）
    shm_endpoint_table_print_stats(NULL);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_endpoint_table_print_stats函数 - 空表
 */
static void test_shm_endpoint_table_print_stats_empty(CuTest* tc) {
    test_setup(tc);
    
    // 打印空表统计
    shm_endpoint_table_print_stats(&g_test_table);
    
    // 验证表状态
    CuAssertIntEquals(tc, 0, g_test_table.count);
    
    test_teardown(tc);
}

/**
 * @brief 测试端点统计信息初始化
 */
static void test_shm_endpoint_stats_initialization(CuTest* tc) {
    test_setup(tc);
    
    // 注册端点
    int result = shm_endpoint_register_base(&g_test_table, TEST_ENDPOINT_NAME, 
                                          TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 查找端点并验证统计信息初始化
    shm_endpoint_t* endpoint = shm_endpoint_find(&g_test_table, TEST_ENDPOINT_NAME);
    CuAssertPtrNotNull(tc, endpoint);
    
    CuAssertIntEquals(tc, 0, endpoint->bytes_sent);
    CuAssertIntEquals(tc, 0, endpoint->bytes_received);
    CuAssertIntEquals(tc, 0, endpoint->send_errors);
    CuAssertIntEquals(tc, 0, endpoint->receive_errors);
    CuAssertIntEquals(tc, 0, endpoint->total_sends);
    CuAssertIntEquals(tc, 0, endpoint->total_receives);
    
    test_teardown(tc);
}

/**
 * @brief 测试最大端点数量边界
 */
static void test_shm_endpoint_max_endpoints_boundary(CuTest* tc) {
    test_setup(tc);
    
    char name[32];
    
    // 精确注册MAX_ENDPOINTS个端点
    for (int i = 0; i < MAX_ENDPOINTS; i++) {
        snprintf(name, sizeof(name), "endpoint_%d", i);
        int result = shm_endpoint_register_base(&g_test_table, name, 
                                              TEST_DATA_SIZE, TEST_QUEUE_SIZE);
        CuAssertIntEquals(tc, SHM_SUCCESS, result);
    }
    
    CuAssertIntEquals(tc, MAX_ENDPOINTS, g_test_table.count);
    
    // 验证所有端点都可以被找到
    for (int i = 0; i < MAX_ENDPOINTS; i++) {
        snprintf(name, sizeof(name), "endpoint_%d", i);
        shm_endpoint_t* endpoint = shm_endpoint_find(&g_test_table, name);
        CuAssertPtrNotNull(tc, endpoint);
        CuAssertStrEquals(tc, name, endpoint->name);
    }
    
    test_teardown(tc);
}

/**
 * @brief 测试端点名称长度边界
 */
static void test_shm_endpoint_name_length_boundary(CuTest* tc) {
    test_setup(tc);
    
    // 测试1个字符的名称
    int result = shm_endpoint_register_base(&g_test_table, "A", 
                                          TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 测试MAX_ENDPOINT_NAME-1个字符的名称
    char long_name[MAX_ENDPOINT_NAME];
    memset(long_name, 'X', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';
    
    result = shm_endpoint_register_base(&g_test_table, long_name, 
                                      TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 验证两个端点都可以被找到
    shm_endpoint_t* endpoint = shm_endpoint_find(&g_test_table, "A");
    CuAssertPtrNotNull(tc, endpoint);
    
    endpoint = shm_endpoint_find(&g_test_table, long_name);
    CuAssertPtrNotNull(tc, endpoint);
    CuAssertStrEquals(tc, long_name, endpoint->name);
    
    test_teardown(tc);
}

/**
 * @brief 测试完整的注册-注销-注册循环
 */
static void test_shm_endpoint_register_unregister_cycle(CuTest* tc) {
    test_setup(tc);
    
    // 第一次注册
    int result = shm_endpoint_register_base(&g_test_table, TEST_ENDPOINT_NAME, 
                                          TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 注销
    result = shm_endpoint_unregister(&g_test_table, TEST_ENDPOINT_NAME);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 重新注册
    result = shm_endpoint_register_base(&g_test_table, TEST_ENDPOINT_NAME, 
                                      TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 验证端点存在且正确
    shm_endpoint_t* endpoint = shm_endpoint_find(&g_test_table, TEST_ENDPOINT_NAME);
    CuAssertPtrNotNull(tc, endpoint);
    CuAssertStrEquals(tc, TEST_ENDPOINT_NAME, endpoint->name);
    
    test_teardown(tc);
}

/**
 * @brief 测试混合操作场景
 */
static void test_shm_endpoint_mixed_operations(CuTest* tc) {
    test_setup(tc);
    
    // 注册多个端点
    int result = shm_endpoint_register_base(&g_test_table, "endpoint1", 
                                          TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    result = shm_endpoint_register_base(&g_test_table, "endpoint2", 
                                      TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    result = shm_endpoint_register_base(&g_test_table, "endpoint3", 
                                      TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    CuAssertIntEquals(tc, 3, g_test_table.count);
    
    // 注销中间的端点
    result = shm_endpoint_unregister(&g_test_table, "endpoint2");
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    CuAssertIntEquals(tc, 2, g_test_table.count);
    
    // 注册新端点
    result = shm_endpoint_register_base(&g_test_table, "endpoint4", 
                                      TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    CuAssertIntEquals(tc, 3, g_test_table.count);
    
    // 验证剩余端点
    shm_endpoint_t* endpoint1 = shm_endpoint_find(&g_test_table, "endpoint1");
    CuAssertPtrNotNull(tc, endpoint1);
    
    shm_endpoint_t* endpoint3 = shm_endpoint_find(&g_test_table, "endpoint3");
    CuAssertPtrNotNull(tc, endpoint3);
    
    shm_endpoint_t* endpoint4 = shm_endpoint_find(&g_test_table, "endpoint4");
    CuAssertPtrNotNull(tc, endpoint4);
    
    shm_endpoint_t* endpoint2 = shm_endpoint_find(&g_test_table, "endpoint2");
    CuAssertPtrEquals(tc, NULL, endpoint2);
    
    test_teardown(tc);
}

/**
 * @brief 获取shm_endpoint模块的测试套件
 * 
 * @return CuSuite* 测试套件指针
 */
CuSuite* ShmEndpointGetSuite() {
    CuSuite* suite = CuSuiteNew();
    
    // 初始化相关测试
    SUITE_ADD_TEST(suite, test_shm_endpoint_table_init_normal);
    SUITE_ADD_TEST(suite, test_shm_endpoint_table_init_invalid_param);
    SUITE_ADD_TEST(suite, test_shm_endpoint_table_init_size_too_small);
    
    // 注册相关测试
    SUITE_ADD_TEST(suite, test_shm_endpoint_register_base_normal);
    SUITE_ADD_TEST(suite, test_shm_endpoint_register_base_invalid_param);
    SUITE_ADD_TEST(suite, test_shm_endpoint_register_base_empty_name);
    SUITE_ADD_TEST(suite, test_shm_endpoint_register_base_long_name);
    SUITE_ADD_TEST(suite, test_shm_endpoint_register_base_boundary_name);
    SUITE_ADD_TEST(suite, test_shm_endpoint_register_base_duplicate);
    SUITE_ADD_TEST(suite, test_shm_endpoint_register_base_table_full);
    SUITE_ADD_TEST(suite, test_shm_endpoint_register_base_insufficient_memory);
    
    // 注销相关测试
    SUITE_ADD_TEST(suite, test_shm_endpoint_unregister_normal);
    SUITE_ADD_TEST(suite, test_shm_endpoint_unregister_invalid_param);
    SUITE_ADD_TEST(suite, test_shm_endpoint_unregister_not_found);
    SUITE_ADD_TEST(suite, test_shm_endpoint_unregister_middle_endpoint);
    
    // 查找相关测试
    SUITE_ADD_TEST(suite, test_shm_endpoint_find_normal);
    SUITE_ADD_TEST(suite, test_shm_endpoint_find_invalid_param);
    SUITE_ADD_TEST(suite, test_shm_endpoint_find_not_found);
    SUITE_ADD_TEST(suite, test_shm_endpoint_find_similar_names);
    
    // 打印函数相关测试
    SUITE_ADD_TEST(suite, test_shm_endpoint_table_print_normal);
    SUITE_ADD_TEST(suite, test_shm_endpoint_table_print_invalid_param);
    SUITE_ADD_TEST(suite, test_shm_endpoint_table_print_empty);
    SUITE_ADD_TEST(suite, test_shm_endpoint_table_print_stats_normal);
    SUITE_ADD_TEST(suite, test_shm_endpoint_table_print_stats_invalid_param);
    SUITE_ADD_TEST(suite, test_shm_endpoint_table_print_stats_empty);
    
    // 统计信息测试
    SUITE_ADD_TEST(suite, test_shm_endpoint_stats_initialization);
    
    // 边界和压力测试
    SUITE_ADD_TEST(suite, test_shm_endpoint_max_endpoints_boundary);
    SUITE_ADD_TEST(suite, test_shm_endpoint_name_length_boundary);
    
    // 集成测试
    SUITE_ADD_TEST(suite, test_shm_endpoint_register_unregister_cycle);
    SUITE_ADD_TEST(suite, test_shm_endpoint_mixed_operations);
    
    return suite;
}
