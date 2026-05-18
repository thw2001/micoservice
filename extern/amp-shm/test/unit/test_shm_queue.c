/**
 * @file test_shm_queue.c
 * @brief 共享内存队列模块单元测试
 * @copyright 2023 AMP-SHM Project, MIT License
 * 
 * 使用CuTest框架对shm_queue模块进行单元测试，覆盖主要功能点。
 * 遵循C语言工程化开发规范和TDD最佳实践。
 */

#include "CuTest.h"
#include "shm_queue.h"
#include "shm_errors.h"
#include "shm_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// 测试用的共享内存大小
#define TEST_SHM_SIZE (1024 * 1024)  // 1MB
#define TEST_DATA_SIZE 256
#define TEST_QUEUE_SIZE 10

// 全局测试变量
static void* g_test_shm_addr = NULL;
static shm_queue_t g_test_queue;

// 函数声明
extern void* shm_queue_calc_msg_position(void* base_addr, const shm_queue_t* queue, uint32_t index);
extern bool shm_queue_validate_params(const shm_queue_t* queue);

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
    
    
    // 初始化测试队列
    int result = shm_queue_init(&g_test_queue, 0, TEST_DATA_SIZE, TEST_QUEUE_SIZE);
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
    memset(&g_test_queue, 0, sizeof(shm_queue_t));
}

/**
 * @brief 测试shm_queue_init函数 - 正常情况
 */
static void test_shm_queue_init_normal(CuTest* tc) {
    test_setup(tc);
    
    shm_queue_t queue;
    int result = shm_queue_init(&queue, 0, TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    
    // 验证结果
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    CuAssertIntEquals(tc, 0, queue.head);
    CuAssertIntEquals(tc, 0, queue.tail);
    CuAssertIntEquals(tc, TEST_QUEUE_SIZE, queue.size);
    CuAssertIntEquals(tc, TEST_DATA_SIZE, queue.data_size);
    CuAssertIntEquals(tc, 0, queue.offset);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_queue_init函数 - 无效参数
 */
static void test_shm_queue_init_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 测试NULL队列指针
    int result = shm_queue_init(NULL, 0, TEST_DATA_SIZE, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试零数据大小
    shm_queue_t queue;
    result = shm_queue_init(&queue, 100, 0, TEST_QUEUE_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试零队列大小
    result = shm_queue_init(&queue, 100, TEST_DATA_SIZE, 0);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_queue_is_empty函数 - 正常情况
 */
static void test_shm_queue_is_empty_normal(CuTest* tc) {
    test_setup(tc);
    
    // 新初始化的队列应该是空的
    int result = shm_queue_is_empty(&g_test_queue);
    CuAssertIntEquals(tc, 1, result);
    
    // 入队一个消息后应该非空
    char test_data[TEST_DATA_SIZE];
    memset(test_data, 0xAA, TEST_DATA_SIZE);
    result = shm_queue_push(g_test_shm_addr, &g_test_queue, test_data, TEST_DATA_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    result = shm_queue_is_empty(&g_test_queue);
    CuAssertIntEquals(tc, 0, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_queue_is_empty函数 - 无效参数
 */
static void test_shm_queue_is_empty_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 测试NULL队列指针
    int result = shm_queue_is_empty(NULL);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_queue_is_full函数 - 正常情况
 */
static void test_shm_queue_is_full_normal(CuTest* tc) {
    test_setup(tc);
    
    // 新初始化的队列应该非满
    int result = shm_queue_is_full(&g_test_queue);
    CuAssertIntEquals(tc, 0, result);
    
    // 填满队列（因为内部为环形队列，所以消息容量是队列大小-1）
    char test_data[TEST_DATA_SIZE];
    memset(test_data, 0xAA, TEST_DATA_SIZE);
    
    for (int i = 0; i < TEST_QUEUE_SIZE - 1; i++) {
        result = shm_queue_push(g_test_shm_addr, &g_test_queue, test_data, TEST_DATA_SIZE);
        CuAssertIntEquals(tc, SHM_SUCCESS, result);
    }
    
    // 现在队列应该满了
    result = shm_queue_is_full(&g_test_queue);
    CuAssertIntEquals(tc, 1, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_queue_is_full函数 - 无效参数
 */
static void test_shm_queue_is_full_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 测试NULL队列指针
    int result = shm_queue_is_full(NULL);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_queue_push函数 - 正常情况
 */
static void test_shm_queue_push_normal(CuTest* tc) {
    test_setup(tc);
    
    char test_data[TEST_DATA_SIZE];
    memset(test_data, 0xAA, TEST_DATA_SIZE);
    
    // 执行测试
    int result = shm_queue_push(g_test_shm_addr, &g_test_queue, test_data, TEST_DATA_SIZE);
    
    // 验证结果
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    CuAssertIntEquals(tc, 1, g_test_queue.head); // head应该前进1
    CuAssertIntEquals(tc, 0, g_test_queue.tail); // tail应该保持不变
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_queue_push函数 - 数据完整性
 */
static void test_shm_queue_push_data_integrity(CuTest* tc) {
    test_setup(tc);
    
    // 准备测试数据
    char send_data[TEST_DATA_SIZE];
    char recv_data[TEST_DATA_SIZE];
    memset(send_data, 0xAA, TEST_DATA_SIZE);
    memset(recv_data, 0, TEST_DATA_SIZE);
    
    // 入队数据
    int result = shm_queue_push(g_test_shm_addr, &g_test_queue, send_data, TEST_DATA_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 出队数据
    result = shm_queue_pop(g_test_shm_addr, &g_test_queue, recv_data, TEST_DATA_SIZE);
    CuAssertIntEquals(tc, TEST_DATA_SIZE, result);
    
    // 验证数据完整性
    CuAssertIntEquals(tc, 0, memcmp(send_data, recv_data, TEST_DATA_SIZE));
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_queue_push函数 - 无效参数
 */
static void test_shm_queue_push_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    char test_data[TEST_DATA_SIZE];
    
    // 测试NULL基地址
    int result = shm_queue_push(NULL, &g_test_queue, test_data, TEST_DATA_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试NULL队列指针
    result = shm_queue_push(g_test_shm_addr, NULL, test_data, TEST_DATA_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试NULL数据指针
    result = shm_queue_push(g_test_shm_addr, &g_test_queue, NULL, TEST_DATA_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试零数据大小
    result = shm_queue_push(g_test_shm_addr, &g_test_queue, test_data, 0);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_queue_push函数 - 数据过大
 */
static void test_shm_queue_push_data_too_large(CuTest* tc) {
    test_setup(tc);
    
    char test_data[TEST_DATA_SIZE * 2]; // 创建过大的数据
    
    // 尝试入队过大的数据
    int result = shm_queue_push(g_test_shm_addr, &g_test_queue, test_data, TEST_DATA_SIZE * 2);
    CuAssertIntEquals(tc, SHM_QUEUE_DATA_TOO_LARGE, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_queue_push函数 - 队列已满
 */
static void test_shm_queue_push_queue_full(CuTest* tc) {
    test_setup(tc);
    
    // 填满队列（因为内部为环形队列，所以消息容量是队列大小-1）
    char test_data[TEST_DATA_SIZE];
    memset(test_data, 0xAA, TEST_DATA_SIZE);
    
    for (int i = 0; i < TEST_QUEUE_SIZE - 1; i++) {
        int result = shm_queue_push(g_test_shm_addr, &g_test_queue, test_data, TEST_DATA_SIZE);
        CuAssertIntEquals(tc, SHM_SUCCESS, result);
    }
    
    // 尝试入队到已满队列
    int result = shm_queue_push(g_test_shm_addr, &g_test_queue, test_data, TEST_DATA_SIZE);
    CuAssertIntEquals(tc, SHM_QUEUE_FULL, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_queue_pop函数 - 正常情况
 */
static void test_shm_queue_pop_normal(CuTest* tc) {
    test_setup(tc);
    
    // 先入队一个消息
    char send_data[TEST_DATA_SIZE];
    char recv_data[TEST_DATA_SIZE];
    memset(send_data, 0xAA, TEST_DATA_SIZE);
    memset(recv_data, 0, TEST_DATA_SIZE);
    
    int result = shm_queue_push(g_test_shm_addr, &g_test_queue, send_data, TEST_DATA_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 执行出队测试
    result = shm_queue_pop(g_test_shm_addr, &g_test_queue, recv_data, TEST_DATA_SIZE);
    
    // 验证结果
    CuAssertIntEquals(tc, TEST_DATA_SIZE, result);
    CuAssertIntEquals(tc, 1, g_test_queue.head); // head保持不变
    CuAssertIntEquals(tc, 1, g_test_queue.tail); // tail应该前进1
    
    // 验证数据完整性
    CuAssertIntEquals(tc, 0, memcmp(send_data, recv_data, TEST_DATA_SIZE));
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_queue_pop函数 - 无效参数
 */
static void test_shm_queue_pop_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    char recv_data[TEST_DATA_SIZE];
    
    // 测试NULL基地址
    int result = shm_queue_pop(NULL, &g_test_queue, recv_data, TEST_DATA_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试NULL队列指针
    result = shm_queue_pop(g_test_shm_addr, NULL, recv_data, TEST_DATA_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试NULL数据指针
    result = shm_queue_pop(g_test_shm_addr, &g_test_queue, NULL, TEST_DATA_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试零缓冲区大小
    result = shm_queue_pop(g_test_shm_addr, &g_test_queue, recv_data, 0);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_queue_pop函数 - 队列为空
 */
static void test_shm_queue_pop_queue_empty(CuTest* tc) {
    test_setup(tc);
    
    char recv_data[TEST_DATA_SIZE];
    
    // 尝试从空队列出队
    int result = shm_queue_pop(g_test_shm_addr, &g_test_queue, recv_data, TEST_DATA_SIZE);
    CuAssertIntEquals(tc, SHM_QUEUE_EMPTY, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试shm_queue_pop函数 - 缓冲区过小
 */
static void test_shm_queue_pop_buffer_too_small(CuTest* tc) {
    test_setup(tc);
    
    // 先入队一个消息
    char send_data[TEST_DATA_SIZE];
    char recv_data[TEST_DATA_SIZE / 2]; // 创建过小的缓冲区
    memset(send_data, 0xAA, TEST_DATA_SIZE);
    memset(recv_data, 0, TEST_DATA_SIZE / 2);
    
    int result = shm_queue_push(g_test_shm_addr, &g_test_queue, send_data, TEST_DATA_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 尝试用太小的缓冲区出队
    result = shm_queue_pop(g_test_shm_addr, &g_test_queue, recv_data, TEST_DATA_SIZE / 2);
    CuAssertIntEquals(tc, SHM_QUEUE_DATA_FLOW, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试零拷贝入队操作 - 正常情况
 */
static void test_shm_queue_push_alloc_normal(CuTest* tc) {
    test_setup(tc);
    
    // 分配零拷贝空间
    shm_base_msg_t* msg = shm_queue_push_alloc(g_test_shm_addr, &g_test_queue, TEST_DATA_SIZE);
    CuAssertPtrNotNull(tc, msg);
    
    // 填充数据
    memset(msg->payload, 0xBB, TEST_DATA_SIZE);
    msg->size = TEST_DATA_SIZE;
    
    // 完成入队
    shm_queue_push_alloc_end(g_test_shm_addr, &g_test_queue);
    
    // 验证队列状态
    CuAssertIntEquals(tc, 1, g_test_queue.head); // head应该前进1
    CuAssertIntEquals(tc, 0, g_test_queue.tail); // tail应该保持不变
    
    test_teardown(tc);
}

/**
 * @brief 测试零拷贝入队操作 - 无效参数
 */
static void test_shm_queue_push_alloc_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 测试NULL基地址
    shm_base_msg_t* msg = shm_queue_push_alloc(NULL, &g_test_queue, TEST_DATA_SIZE);
    CuAssertPtrEquals(tc, NULL, msg);
    
    // 测试NULL队列指针
    msg = shm_queue_push_alloc(g_test_shm_addr, NULL, TEST_DATA_SIZE);
    CuAssertPtrEquals(tc, NULL, msg);
    
    // 测试零数据大小
    msg = shm_queue_push_alloc(g_test_shm_addr, &g_test_queue, 0);
    CuAssertPtrEquals(tc, NULL, msg);
    
    test_teardown(tc);
}

/**
 * @brief 测试零拷贝入队完成操作 - 无效参数
 */
static void test_shm_queue_push_alloc_end_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 测试NULL基地址（应该不会崩溃，但会记录错误）
    shm_queue_push_alloc_end(NULL, &g_test_queue);
    
    // 测试NULL队列指针（应该不会崩溃，但会记录错误）
    shm_queue_push_alloc_end(g_test_shm_addr, NULL);
    
    test_teardown(tc);
}

/**
 * @brief 测试零拷贝出队操作 - 正常情况
 */
static void test_shm_queue_pop_zc_normal(CuTest* tc) {
    test_setup(tc);
    
    // 先入队一个消息（使用普通方式）
    char test_data[TEST_DATA_SIZE];
    memset(test_data, 0xCC, TEST_DATA_SIZE);
    
    int result = shm_queue_push(g_test_shm_addr, &g_test_queue, test_data, TEST_DATA_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 零拷贝出队
    shm_base_msg_t* msg = shm_queue_pop_zc(g_test_shm_addr, &g_test_queue);
    CuAssertPtrNotNull(tc, msg);
    CuAssertIntEquals(tc, TEST_DATA_SIZE, msg->size);
    
    // 验证数据完整性
    CuAssertIntEquals(tc, 0, memcmp(test_data, msg->payload, TEST_DATA_SIZE));
    
    // 完成出队
    shm_queue_pop_zc_end(&g_test_queue);
    
    // 验证队列状态
    CuAssertIntEquals(tc, 1, g_test_queue.head); // head保持不变
    CuAssertIntEquals(tc, 1, g_test_queue.tail); // tail应该前进1
    
    test_teardown(tc);
}

/**
 * @brief 测试零拷贝出队操作 - 无效参数
 */
static void test_shm_queue_pop_zc_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 测试NULL基地址
    shm_base_msg_t* msg = shm_queue_pop_zc(NULL, &g_test_queue);
    CuAssertPtrEquals(tc, NULL, msg);
    
    // 测试NULL队列指针
    msg = shm_queue_pop_zc(g_test_shm_addr, NULL);
    CuAssertPtrEquals(tc, NULL, msg);
    
    test_teardown(tc);
}

/**
 * @brief 测试零拷贝出队完成操作 - 无效参数
 */
static void test_shm_queue_pop_zc_end_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 测试NULL队列指针（应该不会崩溃，但会记录错误）
    shm_queue_pop_zc_end(NULL);
    
    test_teardown(tc);
}

/**
 * @brief 测试零拷贝操作数据完整性
 */
static void test_shm_queue_zero_copy_data_integrity(CuTest* tc) {
    test_setup(tc);
    
    // 零拷贝入队
    shm_base_msg_t* send_msg = shm_queue_push_alloc(g_test_shm_addr, &g_test_queue, TEST_DATA_SIZE);
    CuAssertPtrNotNull(tc, send_msg);
    
    // 填充数据
    memset(send_msg->payload, 0xDD, TEST_DATA_SIZE);
    send_msg->size = TEST_DATA_SIZE;
    
    // 完成入队
    shm_queue_push_alloc_end(g_test_shm_addr, &g_test_queue);
    
    // 零拷贝出队
    shm_base_msg_t* recv_msg = shm_queue_pop_zc(g_test_shm_addr, &g_test_queue);
    CuAssertPtrNotNull(tc, recv_msg);
    
    // 验证数据完整性
    CuAssertIntEquals(tc, TEST_DATA_SIZE, recv_msg->size);
    CuAssertIntEquals(tc, 0, memcmp(send_msg->payload, recv_msg->payload, TEST_DATA_SIZE));
    
    // 完成出队
    shm_queue_pop_zc_end(&g_test_queue);
    
    test_teardown(tc);
}

/**
 * @brief 测试循环队列行为
 */
static void test_shm_queue_circular_behavior(CuTest* tc) {
    test_setup(tc);
    
    char test_data[TEST_DATA_SIZE];
    char recv_data[TEST_DATA_SIZE];
    memset(test_data, 0xEE, TEST_DATA_SIZE);
    memset(recv_data, 0, TEST_DATA_SIZE);
    
    // 填满队列（因为内部为环形队列，所以消息容量是队列大小-1）
    for (int i = 0; i < TEST_QUEUE_SIZE - 1; i++) {
        int result = shm_queue_push(g_test_shm_addr, &g_test_queue, test_data, TEST_DATA_SIZE);
        CuAssertIntEquals(tc, SHM_SUCCESS, result);
    }
    
    // 出队所有消息
    for (int i = 0; i < TEST_QUEUE_SIZE - 1; i++) {
        int result = shm_queue_pop(g_test_shm_addr, &g_test_queue, recv_data, TEST_DATA_SIZE);
        CuAssertIntEquals(tc, TEST_DATA_SIZE, result);
    }
    
    // 现在队列应该又空了
    int result = shm_queue_is_empty(&g_test_queue);
    CuAssertIntEquals(tc, 1, result);
    
    // 可以继续入队（测试循环行为）
    result = shm_queue_push(g_test_shm_addr, &g_test_queue, test_data, TEST_DATA_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试位置计算函数
 */
static void test_shm_queue_calc_msg_position(CuTest* tc) {
    test_setup(tc);
    
    // 测试有效位置计算
    void* position = shm_queue_calc_msg_position(g_test_shm_addr, &g_test_queue, 0);
    CuAssertPtrNotNull(tc, position);
    
    // 测试边界位置
    position = shm_queue_calc_msg_position(g_test_shm_addr, &g_test_queue, TEST_QUEUE_SIZE - 1);
    CuAssertPtrNotNull(tc, position);
    
    // 测试越界位置
    position = shm_queue_calc_msg_position(g_test_shm_addr, &g_test_queue, TEST_QUEUE_SIZE);
    CuAssertPtrEquals(tc, NULL, position);
    
    // 测试无效参数
    position = shm_queue_calc_msg_position(NULL, &g_test_queue, 0);
    CuAssertPtrEquals(tc, NULL, position);
    
    position = shm_queue_calc_msg_position(g_test_shm_addr, NULL, 0);
    CuAssertPtrEquals(tc, NULL, position);
    
    test_teardown(tc);
}

/**
 * @brief 获取shm_queue模块的测试套件
 * 
 * @return CuSuite* 测试套件指针
 */
CuSuite* ShmQueueGetSuite() {
    CuSuite* suite = CuSuiteNew();
    
    // 初始化相关测试
    SUITE_ADD_TEST(suite, test_shm_queue_init_normal);
    SUITE_ADD_TEST(suite, test_shm_queue_init_invalid_param);
    
    // 状态检查相关测试
    SUITE_ADD_TEST(suite, test_shm_queue_is_empty_normal);
    SUITE_ADD_TEST(suite, test_shm_queue_is_empty_invalid_param);
    SUITE_ADD_TEST(suite, test_shm_queue_is_full_normal);
    SUITE_ADD_TEST(suite, test_shm_queue_is_full_invalid_param);
    
    // 普通入队出队测试
    SUITE_ADD_TEST(suite, test_shm_queue_push_normal);
    SUITE_ADD_TEST(suite, test_shm_queue_push_data_integrity);
    SUITE_ADD_TEST(suite, test_shm_queue_push_invalid_param);
    SUITE_ADD_TEST(suite, test_shm_queue_push_data_too_large);
    SUITE_ADD_TEST(suite, test_shm_queue_push_queue_full);
    
    SUITE_ADD_TEST(suite, test_shm_queue_pop_normal);
    SUITE_ADD_TEST(suite, test_shm_queue_pop_invalid_param);
    SUITE_ADD_TEST(suite, test_shm_queue_pop_queue_empty);
    SUITE_ADD_TEST(suite, test_shm_queue_pop_buffer_too_small);
    
    // 零拷贝操作测试
    SUITE_ADD_TEST(suite, test_shm_queue_push_alloc_normal);
    SUITE_ADD_TEST(suite, test_shm_queue_push_alloc_invalid_param);
    SUITE_ADD_TEST(suite, test_shm_queue_push_alloc_end_invalid_param);
    
    SUITE_ADD_TEST(suite, test_shm_queue_pop_zc_normal);
    SUITE_ADD_TEST(suite, test_shm_queue_pop_zc_invalid_param);
    SUITE_ADD_TEST(suite, test_shm_queue_pop_zc_end_invalid_param);
    SUITE_ADD_TEST(suite, test_shm_queue_zero_copy_data_integrity);
    
    // 高级功能测试
    SUITE_ADD_TEST(suite, test_shm_queue_circular_behavior);
    SUITE_ADD_TEST(suite, test_shm_queue_calc_msg_position);
    
    return suite;
}
