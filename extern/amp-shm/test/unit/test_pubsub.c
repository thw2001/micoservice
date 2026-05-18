/**
 * @file test_pubsub.c
 * @brief 发布订阅模块单元测试
 * @copyright 2023 AMP-SHM Project, MIT License
 * 
 * 使用CuTest框架对pubsub模块进行单元测试，覆盖主要功能点。
 * 包括配置初始化、消息发布、系统初始化等功能测试。
 */

#include "CuTest.h"
#include "pubsub.h"
#include "json_config_processor.h"
#include "shm_comm.h"
#include "shm_errors.h"
#include "shm_log.h"
#include "om.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// 测试用的共享内存大小
#define TEST_SHM_SIZE (1024 * 1024)  // 1MB
#define TEST_TOPIC_NAME "test_topic"
#define TEST_PUBER_NAME "test_puber"
#define TEST_SUBER_NAME "test_suber"
#define TEST_MSG_SIZE 128

int gen_topic_puber_channel_name(char* buffer, const char* topic, const char* puber);
int config_pub_sub(pubsub_config_new_t* pcfg);
int cmp_vec_topic(const void* a, const void* b);
int cmp_by_topic(const void* a, const void* b);

// 全局测试变量
static void* g_test_shm_addr = NULL;
static pubsub_config_new_t g_test_config;

// 测试用的JSON配置字符串
static const char* TEST_JSON_CONFIG = 
"{"
"  \"center\": ["
"    {"
"      \"topic\": \"test_topic\","
"      \"len\": 10,"
"      \"msg_size\": 128,"
"      \"topic_priority\": 1,"
"      \"pubers\": ["
"        {"
"          \"name\": \"test_puber\""
"        }"
"      ],"
"      \"subers\": ["
"        {"
"          \"name\": \"test_suber\""
"        }"
"      ]"
"    }"
"  ]"
"}";

// 测试用的配置文件内容
static const char* TEST_CONFIG_FILENAME = "test_config.json";

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
    
    // 初始化pubsub配置
    int result = pubsub_config_new_init(&g_test_config);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    result = shm_comm_init_main(&g_test_config.shm_comm_config, g_test_shm_addr, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 设置日志级别，避免测试输出过多日志
}

/**
 * @brief 测试清理函数
 * 
 * 在每个测试用例执行后调用，清理测试环境。
 */
static void test_teardown(CuTest* tc) {
    // 清理pubsub配置
    int result = pubsub_config_new_deinit(&g_test_config);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 释放共享内存
    if (g_test_shm_addr) {
        free(g_test_shm_addr);
        g_test_shm_addr = NULL;
    }
}

/**
 * @brief 测试pubsub_config_new_init函数 - 正常情况
 */
static void test_pubsub_config_new_init_normal(CuTest* tc) {
    test_setup(tc);
    
    // 创建测试配置对象
    pubsub_config_new_t test_config;
    
    // 执行测试
    int result = pubsub_config_new_init(&test_config);
    
    // 验证结果
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 验证配置对象被正确初始化
    CuAssertIntEquals(tc, 0, test_config.pcfg_json_obj.center.length);
    CuAssertIntEquals(tc, 0, test_config.shm_comm_config.channel_num);
    CuAssertPtrEquals(tc, NULL, test_config.shm_comm_config.ep_table);
    
    // 清理
    pubsub_config_new_deinit(&test_config);
    
    test_teardown(tc);
}

/**
 * @brief 测试pubsub_config_new_init函数 - 无效参数
 */
static void test_pubsub_config_new_init_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 执行测试 - NULL参数
    int result = pubsub_config_new_init(NULL);
    
    // 验证结果
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试pubsub_config_new_deinit函数 - 正常情况
 */
static void test_pubsub_config_new_deinit_normal(CuTest* tc) {
    test_setup(tc);
    
    // 创建并初始化测试配置对象
    pubsub_config_new_t test_config;
    int result = pubsub_config_new_init(&test_config);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 执行测试
    result = pubsub_config_new_deinit(&test_config);
    
    // 验证结果
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试pubsub_config_new_deinit函数 - 无效参数
 */
static void test_pubsub_config_new_deinit_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 执行测试 - NULL参数
    int result = pubsub_config_new_deinit(NULL);
    
    // 验证结果
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试gen_topic_puber_channel_name函数 - 正常情况
 */
static void test_gen_topic_puber_channel_name_normal(CuTest* tc) {
    test_setup(tc);
    
    char buffer[MAX_ENDPOINT_NAME] = {0};
    const char* topic = "test_topic";
    const char* puber = "test_puber";
    
    // 执行测试
    int result = gen_topic_puber_channel_name(buffer, topic, puber);
    
    // 验证结果
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    CuAssertStrEquals(tc, "[topic]test_topic:test_puber", buffer);
    
    test_teardown(tc);
}

/**
 * @brief 测试gen_topic_puber_channel_name函数 - 无效参数
 */
static void test_gen_topic_puber_channel_name_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    char buffer[MAX_ENDPOINT_NAME] = {0};
    const char* topic = "test_topic";
    const char* puber = "test_puber";
    
    // 测试NULL缓冲区
    int result = gen_topic_puber_channel_name(NULL, topic, puber);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试NULL主题
    result = gen_topic_puber_channel_name(buffer, NULL, puber);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试NULL发布者
    result = gen_topic_puber_channel_name(buffer, topic, NULL);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试gen_topic_puber_channel_name函数 - 缓冲区太小
 */
static void test_gen_topic_puber_channel_name_buffer_too_small(CuTest* tc) {
    test_setup(tc);
    
    char small_buffer[10] = {0};  // 故意使用小缓冲区
    const char* long_topic = "very_long_topic_name_that_exceeds_buffer_size";
    const char* long_puber = "very_long_puber_name_that_exceeds_buffer_size";
    
    // 执行测试
    int result = gen_topic_puber_channel_name(small_buffer, long_topic, long_puber);
    
    // 验证结果
    CuAssertIntEquals(tc, SHM_COMM_BUFFER_TOO_SMALL, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试pub_init_config_strs函数 - 正常情况
 */
static void test_pub_init_config_strs_normal(CuTest* tc) {
    test_setup(tc);
    
    const char* json_strs[] = {TEST_JSON_CONFIG};
    int count = 1;
    
    // 执行测试
    int result = pub_init_config_strs(&g_test_config, json_strs, count);
    
    // 验证结果
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    CuAssertTrue(tc, g_test_config.pcfg_json_obj.center.length > 0);
    
    // 验证配置内容
    json_config_center_t* center = &g_test_config.pcfg_json_obj.center.data[0];
    CuAssertStrEquals(tc, "test_topic", center->topic);
    CuAssertIntEquals(tc, 10, center->len);
    CuAssertIntEquals(tc, 128 + sizeof(PubSubHdr), center->msg_size);
    
    test_teardown(tc);
}

/**
 * @brief 测试pub_init_config_strs函数 - 无效参数
 */
static void test_pub_init_config_strs_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    const char* json_strs[] = {TEST_JSON_CONFIG};
    int count = 1;
    
    // 测试NULL配置对象
    int result = pub_init_config_strs(NULL, json_strs, count);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试NULL JSON字符串数组
    result = pub_init_config_strs(&g_test_config, NULL, count);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试零数量
    result = pub_init_config_strs(&g_test_config, json_strs, 0);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试NULL JSON字符串
    const char* null_json_strs[] = {NULL};
    result = pub_init_config_strs(&g_test_config, null_json_strs, 1);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试pub_init_config_strs函数 - 无效JSON格式
 */
static void test_pub_init_config_strs_invalid_json(CuTest* tc) {
    test_setup(tc);
    
    const char* invalid_json = "{invalid json}";
    const char* json_strs[] = {invalid_json};
    int count = 1;
    
    // 执行测试
    int result = pub_init_config_strs(&g_test_config, json_strs, count);
    
    // 验证结果应该失败
    CuAssertTrue(tc, result != SHM_SUCCESS);
    
    test_teardown(tc);
}

/**
 * @brief 测试pub_init_config_files函数 - 正常情况
 */
static void test_pub_init_config_files_normal(CuTest* tc) {
    test_setup(tc);
    
    // 创建临时配置文件
    FILE* config_file = fopen(TEST_CONFIG_FILENAME, "w");
    CuAssertPtrNotNull(tc, config_file);
    fprintf(config_file, "%s", TEST_JSON_CONFIG);
    fclose(config_file);
    
    const char* filenames[] = {TEST_CONFIG_FILENAME};
    int count = 1;
    
    // 执行测试
    int result = pub_init_config_files(&g_test_config, filenames, count);
    
    // 验证结果
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    CuAssertTrue(tc, g_test_config.pcfg_json_obj.center.length > 0);
    
    // 清理临时文件
    remove(TEST_CONFIG_FILENAME);
    
    test_teardown(tc);
}

/**
 * @brief 测试pub_init_config_files函数 - 无效参数
 */
static void test_pub_init_config_files_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    const char* filenames[] = {TEST_CONFIG_FILENAME};
    int count = 1;
    
    // 测试NULL配置对象
    int result = pub_init_config_files(NULL, filenames, count);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试NULL文件名数组
    result = pub_init_config_files(&g_test_config, NULL, count);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试零数量
    result = pub_init_config_files(&g_test_config, filenames, 0);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试NULL文件名
    const char* null_filenames[] = {NULL};
    result = pub_init_config_files(&g_test_config, null_filenames, 1);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试pub_init_config_files函数 - 文件不存在
 */
static void test_pub_init_config_files_file_not_found(CuTest* tc) {
    test_setup(tc);
    
    const char* filenames[] = {"non_existent_file.json"};
    int count = 1;
    
    // 执行测试
    int result = pub_init_config_files(&g_test_config, filenames, count);
    
    // 验证结果应该失败
    CuAssertTrue(tc, result != SHM_SUCCESS);
    
    test_teardown(tc);
}

/**
 * @brief 测试pubsub_cal_mem函数
 */
static void test_pubsub_cal_mem(CuTest* tc) {
    test_setup(tc);
    
    // 首先初始化配置
    const char* json_strs[] = {TEST_JSON_CONFIG};
    int result = pub_init_config_strs(&g_test_config, json_strs, 1);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 执行测试
    int mem_size = pubsub_cal_mem(&g_test_config);
    
    // 验证结果应该大于0
    CuAssertTrue(tc, mem_size > 0);
    
    test_teardown(tc);
}

/**
 * @brief 测试pubsub_init函数 - 正常情况（主进程）
 */
static void test_pubsub_init_main_normal(CuTest* tc) {
    test_setup(tc);
    
    // 执行测试
    int result = pubsub_init(&g_test_config, 1, g_test_shm_addr, TEST_SHM_SIZE);
    
    // 验证结果
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试pubsub_init函数 - 正常情况（从进程）
 */
static void test_pubsub_init_minor_normal(CuTest* tc) {
    test_setup(tc);
    
    // 执行测试
    int result = pubsub_init(&g_test_config, 0, g_test_shm_addr, TEST_SHM_SIZE);
    
    // 验证结果
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试pubsub_init函数 - 无效参数
 */
static void test_pubsub_init_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 测试NULL配置对象
    int result = pubsub_init(NULL, 1, g_test_shm_addr, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试NULL共享内存地址
    result = pubsub_init(&g_test_config, 1, NULL, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试内存大小不足
    result = pubsub_init(&g_test_config, 1, g_test_shm_addr, 0);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试pub_msg函数 - 正常情况
 */
static void test_pub_msg_normal(CuTest* tc) {
    test_setup(tc);
    
    // 首先初始化配置
    const char* json_strs[] = {TEST_JSON_CONFIG};
    int result = pub_init_config_strs(&g_test_config, json_strs, 1);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 初始化pubsub系统
    result = pubsub_init(&g_test_config, 1, g_test_shm_addr, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 准备测试消息
    char test_message[TEST_MSG_SIZE];
    memset(test_message, 0xAA, TEST_MSG_SIZE);
    
    // 执行测试 - 发布消息
    result = pub_msg(&g_test_config, TEST_PUBER_NAME, TEST_TOPIC_NAME, test_message, TEST_MSG_SIZE);
    
    // 验证结果
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试pub_msg函数 - 无效参数
 */
static void test_pub_msg_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 初始化配置
    const char* json_strs[] = {TEST_JSON_CONFIG};
    int result = pub_init_config_strs(&g_test_config, json_strs, 1);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 初始化pubsub系统
    result = pubsub_init(&g_test_config, 1, g_test_shm_addr, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    char test_message[TEST_MSG_SIZE];
    memset(test_message, 0xAA, TEST_MSG_SIZE);
    
    // 测试NULL配置对象
    result = pub_msg(NULL, TEST_PUBER_NAME, TEST_TOPIC_NAME, test_message, TEST_MSG_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试NULL发布者名称
    result = pub_msg(&g_test_config, NULL, TEST_TOPIC_NAME, test_message, TEST_MSG_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试NULL主题名称
    result = pub_msg(&g_test_config, TEST_PUBER_NAME, NULL, test_message, TEST_MSG_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试NULL消息数据
    result = pub_msg(&g_test_config, TEST_PUBER_NAME, TEST_TOPIC_NAME, NULL, TEST_MSG_SIZE);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试零长度消息
    result = pub_msg(&g_test_config, TEST_PUBER_NAME, TEST_TOPIC_NAME, test_message, 0);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试pub_msg函数 - 主题不匹配
 */
static void test_pub_msg_topic_not_found(CuTest* tc) {
    test_setup(tc);
    
    // 初始化配置
    const char* json_strs[] = {TEST_JSON_CONFIG};
    int result = pub_init_config_strs(&g_test_config, json_strs, 1);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 初始化pubsub系统
    result = pubsub_init(&g_test_config, 1, g_test_shm_addr, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 准备测试消息
    char test_message[TEST_MSG_SIZE];
    memset(test_message, 0xAA, TEST_MSG_SIZE);
    
    // 执行测试 - 发布到不存在的主题
    result = pub_msg(&g_test_config, TEST_PUBER_NAME, "non_existent_topic", test_message, TEST_MSG_SIZE);
    
    // 验证结果应该返回有错误（但不是参数错误）
    CuAssertTrue(tc, result != SHM_SUCCESS);
    CuAssertTrue(tc, result != SHM_INVALID_PARAM);
    
    test_teardown(tc);
}


/**
 * @brief 测试config_pub_sub函数 - 正常情况
 */
static void test_config_pub_sub_normal(CuTest* tc) {
    test_setup(tc);
    
    // 初始化配置
    const char* json_strs[] = {TEST_JSON_CONFIG};
    int result = pub_init_config_strs(&g_test_config, json_strs, 1);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 执行配置发布订阅通道
    result = config_pub_sub(&g_test_config);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试config_pub_sub函数 - 无效参数
 */
static void test_config_pub_sub_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 测试NULL配置对象
    int result = config_pub_sub(NULL);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    test_teardown(tc);
}

/**
 * @brief 测试recv_msg函数 - 基本功能
 */
static void test_recv_msg_basic(CuTest* tc) {
    test_setup(tc);
    
    // 初始化配置
    const char* json_strs[] = {TEST_JSON_CONFIG};
    int result = pub_init_config_strs(&g_test_config, json_strs, 1);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 初始化pubsub系统
    result = pubsub_init(&g_test_config, 1, g_test_shm_addr, TEST_SHM_SIZE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 测试recv_msg函数接收消息（循环while 1，需要单独起线程运行）
    // 在实际测试中，可能需要使用多线程或超时机制
    
    test_teardown(tc);
}

/**
 * @brief 测试recv_msg函数 - 无效参数
 */
static void test_recv_msg_invalid_param(CuTest* tc) {
    test_setup(tc);
    
    // 测试NULL配置对象
    int result = recv_msg(NULL, TEST_SUBER_NAME);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    // 测试NULL接收者名称
    result = recv_msg(&g_test_config, NULL);
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, result);
    
    test_teardown(tc);
}

/**
 * @brief 获取pubsub模块的测试套件
 * 
 * @return CuSuite* 测试套件指针
 */
CuSuite* PubSubGetSuite() {
    CuSuite* suite = CuSuiteNew();
    
    // 新配置对象初始化和释放测试
    SUITE_ADD_TEST(suite, test_pubsub_config_new_init_normal);
    SUITE_ADD_TEST(suite, test_pubsub_config_new_init_invalid_param);
    SUITE_ADD_TEST(suite, test_pubsub_config_new_deinit_normal);
    SUITE_ADD_TEST(suite, test_pubsub_config_new_deinit_invalid_param);
    
    // 内部函数测试
    SUITE_ADD_TEST(suite, test_gen_topic_puber_channel_name_normal);
    SUITE_ADD_TEST(suite, test_gen_topic_puber_channel_name_invalid_param);
    // SUITE_ADD_TEST(suite, test_gen_topic_puber_channel_name_buffer_too_small);
    
    // 配置初始化测试
    SUITE_ADD_TEST(suite, test_pub_init_config_strs_normal);
    SUITE_ADD_TEST(suite, test_pub_init_config_strs_invalid_param);
    SUITE_ADD_TEST(suite, test_pub_init_config_strs_invalid_json);
    SUITE_ADD_TEST(suite, test_pub_init_config_files_normal);
    SUITE_ADD_TEST(suite, test_pub_init_config_files_invalid_param);
    SUITE_ADD_TEST(suite, test_pub_init_config_files_file_not_found);
    
    
    // 通道配置测试
    SUITE_ADD_TEST(suite, test_config_pub_sub_normal);
    SUITE_ADD_TEST(suite, test_config_pub_sub_invalid_param);
    
    // 系统初始化测试
    SUITE_ADD_TEST(suite, test_pubsub_cal_mem);
    SUITE_ADD_TEST(suite, test_pubsub_init_main_normal);
    SUITE_ADD_TEST(suite, test_pubsub_init_minor_normal);
    SUITE_ADD_TEST(suite, test_pubsub_init_invalid_param);
    
    // 消息发布测试
    SUITE_ADD_TEST(suite, test_pub_msg_normal);
    SUITE_ADD_TEST(suite, test_pub_msg_invalid_param);
    SUITE_ADD_TEST(suite, test_pub_msg_topic_not_found);
    
    // 消息接收测试
    SUITE_ADD_TEST(suite, test_recv_msg_basic);
    SUITE_ADD_TEST(suite, test_recv_msg_invalid_param);
    
    return suite;
}
