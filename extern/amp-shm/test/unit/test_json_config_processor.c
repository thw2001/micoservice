/*
 * @file test_json_config_processor.c
 * @brief JSON配置处理模块单元测试
 * @copyright 2023 AMP-SHM Project, MIT License
 * 
 * 测试JSON配置处理模块的各项功能，包括序列化、反序列化、合并、验证等。
 */

#include "json_config_processor.h"
#include "shm_log.h"
#include "CuTest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int json_config_suber_init(json_config_suber_t* obj);
int json_config_puber_init(json_config_puber_t* obj);
int json_config_center_init(json_config_center_t* obj);
int json_config_suber_deinit(json_config_suber_t* obj);
int json_config_puber_deinit(json_config_puber_t* obj);
int json_config_center_deinit(json_config_center_t* obj);

int json_config_suber_serialize(const json_config_suber_t* obj, cJSON* json);
int json_config_puber_serialize(const json_config_puber_t* obj, cJSON* json);
int json_config_center_serialize(const json_config_center_t* obj, cJSON* json);
int json_config_serialize(const json_config_t* config, cJSON* json);

int json_config_suber_deserialize(cJSON* json, json_config_suber_t* obj);
int json_config_puber_deserialize(cJSON* json, json_config_puber_t* obj);
int json_config_center_deserialize(cJSON* json, json_config_center_t* obj);
int json_config_deserialize(cJSON* json, json_config_t* config);

int json_config_center_copy(const json_config_center_t* src, json_config_center_t* dest);
int json_config_center_equals(const json_config_center_t* center1, const json_config_center_t* center2);
int json_config_center_validate(const json_config_center_t* center);
int json_config_center_merge(json_config_center_t* dest, const json_config_center_t* src, merge_strategy_t strategy);
int json_config_suber_exists(const vec_json_config_suber_t* subers, const char* name);
int json_config_puber_exists(const vec_json_config_puber_t* pubers, const char* name);

int cmp_by_topic(const void* a, const void* b);
int safe_strdup(const char* src, char** dest);
bool is_valid_topic_name(const char* topic);
bool is_valid_endpoint_name(const char* name);

/* ========================= 辅助函数 ========================= */

/**
 * @brief 创建测试用的JSON配置对象
 * @param[in] topic_count 主题数量
 * @return 配置对象指针
 */
static json_config_t* create_test_config(int topic_count)
{
    json_config_t* config = (json_config_t*)malloc(sizeof(json_config_t));
    if (config == NULL) {
        return NULL;
    }
    
    json_config_init(config);
    
    for (int i = 0; i < topic_count; i++) {
        json_config_center_t center;
        json_config_center_init(&center);
        
        // 设置主题信息
        char topic_name[32];
        snprintf(topic_name, sizeof(topic_name), "test_topic_%d", i);
        center.topic = strdup(topic_name);
        center.len = 100 + i * 10;
        center.topic_priority = i;
        center.msg_size = 1024 + i * 256;
        
        // 添加订阅者
        for (int j = 0; j < 2; j++) {
            json_config_suber_t suber;
            json_config_suber_init(&suber);
            char suber_name[64];
            snprintf(suber_name, sizeof(suber_name), "suber_%d_%d", i, j);
            suber.name = strdup(suber_name);
            vec_push(&center.subers, suber);
        }
        
        // 添加发布者
        for (int j = 0; j < 2; j++) {
            json_config_puber_t puber;
            json_config_puber_init(&puber);
            char puber_name[64];
            snprintf(puber_name, sizeof(puber_name), "puber_%d_%d", i, j);
            puber.name = strdup(puber_name);
            vec_push(&center.pubers, puber);
        }
        
        vec_push(&config->center, center);
    }
    
    return config;
}

/**
 * @brief 创建指定参数的测试配置对象（扩展版本）
 * @param[in] topic_name 主题名称
 * @param[in] suber_count 订阅者数量
 * @param[in] puber_count 发布者数量
 * @param[in] len 队列长度
 * @param[in] priority 主题优先级
 * @param[in] msg_size 消息大小
 * @param[in] config_prefix 配置前缀，用于确保名称唯一性
 * @return 配置对象指针
 */
static json_config_t* create_test_config_with_details_ex(const char* topic_name, 
                                                        int suber_count, 
                                                        int puber_count,
                                                        int len, 
                                                        int priority, 
                                                        int msg_size,
                                                        const char* config_prefix)
{
    json_config_t* config = (json_config_t*)malloc(sizeof(json_config_t));
    if (config == NULL) {
        return NULL;
    }
    
    json_config_init(config);
    
    json_config_center_t center;
    json_config_center_init(&center);
    
    // 设置主题信息
    center.topic = strdup(topic_name);
    center.len = len;
    center.topic_priority = priority;
    center.msg_size = msg_size;
    
    // 添加订阅者
    for (int i = 0; i < suber_count; i++) {
        json_config_suber_t suber;
        json_config_suber_init(&suber);
        char suber_name[64];
        // 使用配置前缀确保名称唯一性
        snprintf(suber_name, sizeof(suber_name), "suber_%s_%s_%d", config_prefix, topic_name, i);
        suber.name = strdup(suber_name);
        vec_push(&center.subers, suber);
    }
    
    // 添加发布者
    for (int i = 0; i < puber_count; i++) {
        json_config_puber_t puber;
        json_config_puber_init(&puber);
        char puber_name[64];
        // 使用配置前缀确保名称唯一性
        snprintf(puber_name, sizeof(puber_name), "puber_%s_%s_%d", config_prefix, topic_name, i);
        puber.name = strdup(puber_name);
        vec_push(&center.pubers, puber);
    }
    
    vec_push(&config->center, center);
    return config;
}

/**
 * @brief 创建指定参数的测试配置对象
 * @param[in] topic_name 主题名称
 * @param[in] suber_count 订阅者数量
 * @param[in] puber_count 发布者数量
 * @param[in] len 队列长度
 * @param[in] priority 主题优先级
 * @param[in] msg_size 消息大小
 * @return 配置对象指针
 */
static json_config_t* create_test_config_with_details(const char* topic_name, 
                                                     int suber_count, 
                                                     int puber_count,
                                                     int len, 
                                                     int priority, 
                                                     int msg_size)
{
    return create_test_config_with_details_ex(topic_name, suber_count, puber_count, 
                                            len, priority, msg_size, "default");
}

/**
 * @brief 释放测试用的JSON配置对象
 * @param[in] config 配置对象指针
 */
static void free_test_config(json_config_t* config)
{
    if (config != NULL) {
        json_config_deinit(config);
        free(config);
    }
}

/**
 * @brief 比较两个配置对象是否相等
 * @param[in] tc 测试用例指针
 * @param[in] config1 第一个配置对象
 * @param[in] config2 第二个配置对象
 * @param[in] msg 错误消息
 */
static void assert_config_equals(CuTest* tc, const json_config_t* config1, 
                                 const json_config_t* config2, const char* msg)
{
    CuAssertIntEquals_Msg(tc, msg, config1->center.length, config2->center.length);
    
    for (int i = 0; i < config1->center.length; i++) {
        const json_config_center_t* center1 = &config1->center.data[i];
        const json_config_center_t* center2 = &config2->center.data[i];
        
        CuAssertStrEquals_Msg(tc, msg, center1->topic, center2->topic);
        CuAssertIntEquals_Msg(tc, msg, center1->len, center2->len);
        CuAssertIntEquals_Msg(tc, msg, center1->topic_priority, center2->topic_priority);
        CuAssertIntEquals_Msg(tc, msg, center1->msg_size, center2->msg_size);
        
        CuAssertIntEquals_Msg(tc, msg, center1->subers.length, center2->subers.length);
        CuAssertIntEquals_Msg(tc, msg, center1->pubers.length, center2->pubers.length);
    }
}

/* ========================= 测试用例 ========================= */

/**
 * @brief 测试配置对象的初始化和清理
 * @param[in] tc 测试用例指针
 */
void test_json_config_init_deinit(CuTest* tc)
{
    json_config_t config;
    
    // 测试初始化
    int result = json_config_init(&config);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    CuAssertIntEquals(tc, 0, config.center.length);
    
    // 测试清理
    result = json_config_deinit(&config);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
}

/**
 * @brief 测试JSON序列化和反序列化
 * @param[in] tc 测试用例指针
 */
void test_json_config_serialize_deserialize(CuTest* tc)
{
    // 创建测试配置
    json_config_t* config1 = create_test_config(3);
    CuAssertPtrNotNull(tc, config1);
    
    // 序列化为JSON字符串
    char* json_str = json_config_to_str(config1);
    CuAssertPtrNotNull(tc, json_str);
    CuAssertTrue(tc, strlen(json_str) > 0);
    
    // 从JSON字符串反序列化
    json_config_t config2;
    json_config_init(&config2);
    
    int result = json_config_from_str(&config2, json_str);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 验证配置是否相等
    assert_config_equals(tc, config1, &config2, "Config serialization/deserialization failed");
    
    // 清理资源
    free(json_str);
    json_config_deinit(&config2);
    free_test_config(config1);
}

/**
 * @brief 测试JSON文件读写
 * @param[in] tc 测试用例指针
 */
void test_json_config_file_io(CuTest* tc)
{
    // 创建测试配置
    json_config_t* config1 = create_test_config(2);
    CuAssertPtrNotNull(tc, config1);
    
    // 保存到文件
    const char* filename = "test_config.json";
    int result = json_config_to_file(config1, filename);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 从文件加载
    json_config_t config2;
    json_config_init(&config2);
    
    result = json_config_from_file(&config2, filename);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 验证配置是否相等
    assert_config_equals(tc, config1, &config2, "Config file I/O failed");
    
    // 清理资源
    json_config_deinit(&config2);
    free_test_config(config1);
    remove(filename); // 删除测试文件
}

/**
 * @brief 测试配置合并功能 - NULL参数处理
 * @param[in] tc 测试用例指针
 */
void test_json_config_merge_null_params(CuTest* tc)
{
    // Setup: 创建有效配置
    json_config_t* config = create_test_config(1);
    CuAssertPtrNotNull(tc, config);
    
    // Exercise & Verification: 测试各种NULL参数组合
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, json_config_merge(NULL, config, MERGE_STRATEGY_APPEND));
    CuAssertIntEquals(tc, SHM_INVALID_PARAM, json_config_merge(config, NULL, MERGE_STRATEGY_APPEND));
    
    // Teardown
    free_test_config(config);
}

/**
 * @brief 测试配置合并功能 - 覆盖策略
 * @param[in] tc 测试用例指针
 */
void test_json_config_merge_overwrite_strategy(CuTest* tc)
{
    // Setup: 创建两个相同主题但不同参数的配置
    json_config_t* config1 = create_test_config_with_details("test_topic", 2, 2, 100, 1, 1024);
    CuAssertPtrNotNull(tc, config1);
    
    json_config_t* config2 = create_test_config_with_details("test_topic", 1, 1, 200, 2, 2048);
    CuAssertPtrNotNull(tc, config2);
    
    json_config_t merged_config;
    json_config_init(&merged_config);
    
    // Exercise: 使用覆盖策略合并
    int result = json_config_merge(&merged_config, config1, MERGE_STRATEGY_OVERWRITE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    result = json_config_merge(&merged_config, config2, MERGE_STRATEGY_OVERWRITE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // Verification: 验证配置被完全覆盖
    CuAssertIntEquals(tc, 1, merged_config.center.length);
    const json_config_center_t* center = &merged_config.center.data[0];
    CuAssertIntEquals(tc, 200, center->len);        // 被覆盖
    CuAssertIntEquals(tc, 2, center->topic_priority); // 被覆盖
    CuAssertIntEquals(tc, 2048, center->msg_size);   // 被覆盖
    CuAssertIntEquals(tc, 1, center->subers.length); // 被覆盖
    CuAssertIntEquals(tc, 1, center->pubers.length); // 被覆盖
    
    // Teardown
    json_config_deinit(&merged_config);
    free_test_config(config1);
    free_test_config(config2);
}

/**
 * @brief 测试配置合并功能 - 追加策略
 * @param[in] tc 测试用例指针
 */
void test_json_config_merge_append_strategy(CuTest* tc)
{
    // Setup: 创建两个相同主题但不同订阅者/发布者的配置
    json_config_t* config1 = create_test_config_with_details_ex("test_topic", 2, 1, 100, 1, 1024, "config1");
    CuAssertPtrNotNull(tc, config1);
    
    json_config_t* config2 = create_test_config_with_details_ex("test_topic", 1, 2, 100, 1, 1024, "config2");
    CuAssertPtrNotNull(tc, config2);
    
    json_config_t merged_config;
    json_config_init(&merged_config);
    
    // Exercise: 使用追加策略合并
    int result = json_config_merge(&merged_config, config1, MERGE_STRATEGY_APPEND);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    result = json_config_merge(&merged_config, config2, MERGE_STRATEGY_APPEND);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // Verification: 验证参数保持不变，订阅者和发布者被合并
    CuAssertIntEquals(tc, 1, merged_config.center.length);
    const json_config_center_t* center = &merged_config.center.data[0];
    CuAssertIntEquals(tc, 100, center->len);        // 保持不变
    CuAssertIntEquals(tc, 1, center->topic_priority); // 保持不变
    CuAssertIntEquals(tc, 1024, center->msg_size);   // 保持不变
    CuAssertIntEquals(tc, 3, center->subers.length); // 2 + 1 = 3
    CuAssertIntEquals(tc, 3, center->pubers.length); // 1 + 2 = 3
    
    // Teardown
    json_config_deinit(&merged_config);
    free_test_config(config1);
    free_test_config(config2);
}

/**
 * @brief 测试配置合并功能 - 智能合并策略
 * @param[in] tc 测试用例指针
 */
void test_json_config_merge_merge_strategy(CuTest* tc)
{
    // Setup: 创建两个相同主题和参数的配置
    json_config_t* config1 = create_test_config_with_details_ex("test_topic", 2, 1, 100, 1, 1024, "config1");
    CuAssertPtrNotNull(tc, config1);
    
    json_config_t* config2 = create_test_config_with_details_ex("test_topic", 1, 1, 100, 1, 1024, "config2");
    CuAssertPtrNotNull(tc, config2);
    
    json_config_t merged_config;
    json_config_init(&merged_config);
    
    // Exercise: 使用智能合并策略合并
    int result = json_config_merge(&merged_config, config1, MERGE_STRATEGY_MERGE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    result = json_config_merge(&merged_config, config2, MERGE_STRATEGY_MERGE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // Verification: 验证合并成功
    CuAssertIntEquals(tc, 1, merged_config.center.length);
    const json_config_center_t* center = &merged_config.center.data[0];
    CuAssertIntEquals(tc, 100, center->len);        // 保持不变
    CuAssertIntEquals(tc, 1, center->topic_priority); // 保持不变
    CuAssertIntEquals(tc, 1024, center->msg_size);   // 保持不变
    CuAssertIntEquals(tc, 3, center->subers.length); // 1 + 1 = 2
    CuAssertIntEquals(tc, 2, center->pubers.length); // 1 + 1 = 2
    
    // Teardown
    json_config_deinit(&merged_config);
    free_test_config(config1);
    free_test_config(config2);
}

/**
 * @brief 测试配置合并功能 - 合并冲突
 * @param[in] tc 测试用例指针
 */
void test_json_config_merge_merge_conflict(CuTest* tc)
{
    // Setup: 创建两个相同主题但不同参数的配置
    json_config_t* config1 = create_test_config_with_details("test_topic", 1, 1, 100, 1, 1024);
    CuAssertPtrNotNull(tc, config1);
    
    json_config_t* config2 = create_test_config_with_details("test_topic", 1, 1, 200, 1, 1024); // len不同
    CuAssertPtrNotNull(tc, config2);
    
    json_config_t merged_config;
    json_config_init(&merged_config);
    
    // Exercise: 先合并第一个配置
    CuAssertIntEquals(tc, SHM_SUCCESS, json_config_merge(&merged_config, config1, MERGE_STRATEGY_MERGE));
    
    // Verification: 第二个配置应该因为参数冲突而合并失败
    CuAssertIntEquals(tc, JSON_CONFIG_MERGE_CONFLICT, 
                     json_config_merge(&merged_config, config2, MERGE_STRATEGY_MERGE));
    
    // Teardown
    json_config_deinit(&merged_config);
    free_test_config(config1);
    free_test_config(config2);
}

/**
 * @brief 测试配置合并功能 - 不同主题合并
 * @param[in] tc 测试用例指针
 */
void test_json_config_merge_different_topics(CuTest* tc)
{
    // Setup: 创建两个不同主题的配置
    json_config_t* config1 = create_test_config_with_details("topic1", 1, 1, 100, 1, 1024);
    CuAssertPtrNotNull(tc, config1);
    
    json_config_t* config2 = create_test_config_with_details("topic2", 1, 1, 200, 2, 2048);
    CuAssertPtrNotNull(tc, config2);
    
    json_config_t merged_config;
    json_config_init(&merged_config);
    
    // Exercise: 合并不同主题的配置
    int result = json_config_merge(&merged_config, config1, MERGE_STRATEGY_MERGE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    result = json_config_merge(&merged_config, config2, MERGE_STRATEGY_MERGE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // Verification: 验证两个主题都存在，参数保持各自原值
    CuAssertIntEquals(tc, 2, merged_config.center.length);
    
    const json_config_center_t* center1 = json_config_get_topic(&merged_config, "topic1");
    CuAssertPtrNotNull(tc, center1);
    CuAssertIntEquals(tc, 100, center1->len);
    CuAssertIntEquals(tc, 1, center1->topic_priority);
    CuAssertIntEquals(tc, 1024, center1->msg_size);
    
    const json_config_center_t* center2 = json_config_get_topic(&merged_config, "topic2");
    CuAssertPtrNotNull(tc, center2);
    CuAssertIntEquals(tc, 200, center2->len);
    CuAssertIntEquals(tc, 2, center2->topic_priority);
    CuAssertIntEquals(tc, 2048, center2->msg_size);
    
    // Teardown
    json_config_deinit(&merged_config);
    free_test_config(config1);
    free_test_config(config2);
}

/**
 * @brief 测试配置合并功能 - 空配置合并
 * @param[in] tc 测试用例指针
 */
void test_json_config_merge_empty_config(CuTest* tc)
{
    // Setup: 创建一个配置和一个空配置
    json_config_t* config1 = create_test_config_with_details("test_topic", 1, 1, 100, 1, 1024);
    CuAssertPtrNotNull(tc, config1);
    
    json_config_t empty_config;
    json_config_init(&empty_config);
    
    json_config_t merged_config;
    json_config_init(&merged_config);
    
    // Exercise: 先合并空配置，再合并正常配置
    int result = json_config_merge(&merged_config, &empty_config, MERGE_STRATEGY_APPEND);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    result = json_config_merge(&merged_config, config1, MERGE_STRATEGY_APPEND);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // Verification: 验证最终结果包含正常配置的内容
    CuAssertIntEquals(tc, 1, merged_config.center.length);
    const json_config_center_t* center = &merged_config.center.data[0];
    CuAssertStrEquals(tc, "test_topic", center->topic);
    CuAssertIntEquals(tc, 1, center->subers.length);
    CuAssertIntEquals(tc, 1, center->pubers.length);
    
    // Teardown
    json_config_deinit(&merged_config);
    json_config_deinit(&empty_config);
    free_test_config(config1);
}

/**
 * @brief 测试配置合并功能 - 去重功能
 * @param[in] tc 测试用例指针
 */
void test_json_config_merge_deduplication(CuTest* tc)
{
    // Setup: 创建包含重复订阅者/发布者的配置
    json_config_t* config1 = create_test_config_with_details("test_topic", 1, 1, 100, 1, 1024);
    CuAssertPtrNotNull(tc, config1);
    
    json_config_t* config2 = create_test_config_with_details("test_topic", 1, 1, 100, 1, 1024);
    CuAssertPtrNotNull(tc, config2);
    
    // 设置相同的订阅者/发布者名称以测试去重
    free(config1->center.data[0].subers.data[0].name);
    config1->center.data[0].subers.data[0].name = strdup("same_suber");
    
    free(config1->center.data[0].pubers.data[0].name);
    config1->center.data[0].pubers.data[0].name = strdup("same_puber");
    
    free(config2->center.data[0].subers.data[0].name);
    config2->center.data[0].subers.data[0].name = strdup("same_suber");
    
    free(config2->center.data[0].pubers.data[0].name);
    config2->center.data[0].pubers.data[0].name = strdup("same_puber");
    
    json_config_t merged_config;
    json_config_init(&merged_config);
    
    // Exercise: 使用追加策略合并
    int result = json_config_merge(&merged_config, config1, MERGE_STRATEGY_APPEND);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    result = json_config_merge(&merged_config, config2, MERGE_STRATEGY_APPEND);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // Verification: 验证重复的订阅者/发布者被去重
    CuAssertIntEquals(tc, 1, merged_config.center.length);
    const json_config_center_t* center = &merged_config.center.data[0];
    CuAssertIntEquals(tc, 1, center->subers.length); // 去重后仍为1
    CuAssertIntEquals(tc, 1, center->pubers.length); // 去重后仍为1
    CuAssertStrEquals(tc, "same_suber", center->subers.data[0].name);
    CuAssertStrEquals(tc, "same_puber", center->pubers.data[0].name);
    
    // Teardown
    json_config_deinit(&merged_config);
    free_test_config(config1);
    free_test_config(config2);
}

/**
 * @brief 测试配置验证功能
 * @param[in] tc 测试用例指针
 */
void test_json_config_validate(CuTest* tc)
{
    // 创建有效的测试配置
    json_config_t* valid_config = create_test_config(1);
    CuAssertPtrNotNull(tc, valid_config);
    
    // 验证有效配置
    int result = json_config_validate(valid_config);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 创建无效的测试配置（空配置）
    json_config_t empty_config;
    json_config_init(&empty_config);
    
    result = json_config_validate(&empty_config);
    CuAssertIntEquals(tc, JSON_CONFIG_VALIDATION_FAIL, result);
    
    // 创建无效的测试配置（重复主题）
    json_config_t* duplicate_config = create_test_config(1);
    CuAssertPtrNotNull(tc, duplicate_config);
    
    // 添加重复的主题
    json_config_center_t center;
    json_config_center_copy(&duplicate_config->center.data[0], &center);
    vec_push(&duplicate_config->center, center);
    
    result = json_config_validate(duplicate_config);
    CuAssertIntEquals(tc, JSON_CONFIG_VALIDATION_FAIL, result);
    
    // 清理资源
    json_config_deinit(&empty_config);
    free_test_config(valid_config);
    free_test_config(duplicate_config);
}

/**
 * @brief 测试配置查询功能
 * @param[in] tc 测试用例指针
 */
void test_json_config_query(CuTest* tc)
{
    // 创建测试配置
    json_config_t* config = create_test_config(3);
    CuAssertPtrNotNull(tc, config);
    
    // 测试主题数量
    int count = json_config_get_topic_count(config);
    CuAssertIntEquals(tc, 3, count);
    
    // 测试主题存在性检查
    bool exists = json_config_has_topic(config, "test_topic_0");
    CuAssertTrue(tc, exists);
    
    exists = json_config_has_topic(config, "nonexistent_topic");
    CuAssertTrue(tc, !exists);
    
    // 测试获取主题配置
    const json_config_center_t* center = json_config_get_topic(config, "test_topic_1");
    CuAssertPtrNotNull(tc, center);
    CuAssertStrEquals(tc, "test_topic_1", center->topic);
    
    center = json_config_get_topic(config, "nonexistent_topic");
    CuAssert(tc, "center ptr should null", NULL == center);
    
    // 清理资源
    free_test_config(config);
}

/**
 * @brief 测试配置复制和比较功能
 * @param[in] tc 测试用例指针
 */
void test_json_config_copy_compare(CuTest* tc)
{
    // 创建测试配置
    json_config_t* config1 = create_test_config(2);
    CuAssertPtrNotNull(tc, config1);
    
    // 复制配置
    json_config_t config2;
    json_config_init(&config2);
    
    int result = json_config_copy(config1, &config2);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 比较配置
    bool equal = json_config_equals(config1, &config2);
    CuAssertTrue(tc, equal);
    
    // 修改配置2，使其不相等
    config2.center.data[0].len = 999;
    
    equal = json_config_equals(config1, &config2);
    CuAssertTrue(tc, !equal);
    
    // 清理资源
    json_config_deinit(&config2);
    free_test_config(config1);
}

/**
 * @brief 测试多文件合并功能
 * @param[in] tc 测试用例指针
 */
void test_json_config_merge_files(CuTest* tc)
{
    // 创建两个测试配置
    json_config_t* config1 = create_test_config(1);
    CuAssertPtrNotNull(tc, config1);
    
    json_config_t* config2 = create_test_config(1);
    CuAssertPtrNotNull(tc, config2);
    
    // 修改第二个配置的主题名称
    strcpy(config2->center.data[0].topic, "merged_topic");
    
    // 保存到文件
    const char* filenames[2] = {"test_config1.json", "test_config2.json"};
    
    int result = json_config_to_file(config1, filenames[0]);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    result = json_config_to_file(config2, filenames[1]);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 从多个文件合并配置
    json_config_t merged_config;
    json_config_init(&merged_config);
    
    result = json_config_merge_files(&merged_config, filenames, 2, MERGE_STRATEGY_MERGE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 验证合并结果
    CuAssertIntEquals(tc, 2, merged_config.center.length);
    CuAssertTrue(tc, json_config_has_topic(&merged_config, "test_topic_0"));
    CuAssertTrue(tc, json_config_has_topic(&merged_config, "merged_topic"));
    
    // 清理资源
    json_config_deinit(&merged_config);
    free_test_config(config1);
    free_test_config(config2);
    remove(filenames[0]);
    remove(filenames[1]);
}

/**
 * @brief 测试多字符串合并功能
 * @param[in] tc 测试用例指针
 */
void test_json_config_merge_strs(CuTest* tc)
{
    // 创建两个测试配置
    json_config_t* config1 = create_test_config(1);
    CuAssertPtrNotNull(tc, config1);
    
    json_config_t* config2 = create_test_config(1);
    CuAssertPtrNotNull(tc, config2);
    
    // 修改第二个配置的主题名称
    strcpy(config2->center.data[0].topic, "merged_topic");
    
    // 序列化为JSON字符串
    char* json_str1 = json_config_to_str(config1);
    CuAssertPtrNotNull(tc, json_str1);
    
    char* json_str2 = json_config_to_str(config2);
    CuAssertPtrNotNull(tc, json_str2);
    
    // 从多个字符串合并配置
    const char* json_strs[2] = {json_str1, json_str2};
    
    json_config_t merged_config;
    json_config_init(&merged_config);
    
    int result = json_config_merge_strs(&merged_config, json_strs, 2, MERGE_STRATEGY_MERGE);
    CuAssertIntEquals(tc, SHM_SUCCESS, result);
    
    // 验证合并结果
    CuAssertIntEquals(tc, 2, merged_config.center.length);
    CuAssertTrue(tc, json_config_has_topic(&merged_config, "test_topic_0"));
    CuAssertTrue(tc, json_config_has_topic(&merged_config, "merged_topic"));
    
    // 清理资源
    json_config_deinit(&merged_config);
    free(json_str1);
    free(json_str2);
    free_test_config(config1);
    free_test_config(config2);
}

/* ========================= 测试套件 ========================= */

/**
 * @brief 添加所有测试用例到测试套件
 * @return 测试套件指针
 */
CuSuite* JsonConfigProcessorGetSuite()
{
    CuSuite* suite = CuSuiteNew();
    
    SUITE_ADD_TEST(suite, test_json_config_init_deinit);
    SUITE_ADD_TEST(suite, test_json_config_serialize_deserialize);
    SUITE_ADD_TEST(suite, test_json_config_file_io);
    
    // 添加新的合并功能测试用例
    SUITE_ADD_TEST(suite, test_json_config_merge_null_params);
    SUITE_ADD_TEST(suite, test_json_config_merge_overwrite_strategy);
    SUITE_ADD_TEST(suite, test_json_config_merge_append_strategy);
    SUITE_ADD_TEST(suite, test_json_config_merge_merge_strategy);
    SUITE_ADD_TEST(suite, test_json_config_merge_merge_conflict);
    SUITE_ADD_TEST(suite, test_json_config_merge_different_topics);
    SUITE_ADD_TEST(suite, test_json_config_merge_empty_config);
    SUITE_ADD_TEST(suite, test_json_config_merge_deduplication);
    
    SUITE_ADD_TEST(suite, test_json_config_validate);
    SUITE_ADD_TEST(suite, test_json_config_query);
    SUITE_ADD_TEST(suite, test_json_config_copy_compare);
    SUITE_ADD_TEST(suite, test_json_config_merge_files);
    SUITE_ADD_TEST(suite, test_json_config_merge_strs);
    
    return suite;
}
