/*
 * @file json_config_processor.c
 * @brief JSON配置处理模块实现
 * @copyright 2023 AMP-SHM Project, MIT License
 * 
 * 提供统一的JSON配置序列化、反序列化、合并、验证功能实现。
 */

#include "json_config_processor.h"
#include "shm_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>

/* ========================= 错误码定义 ========================= */

/* JSON配置处理模块错误码已在shm_errors.h中定义 */

/* ========================= 内部函数声明 ========================= */

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

/* ========================= 工具函数实现 ========================= */

/**
 * @brief 安全的字符串复制函数
 * @param[in] src 源字符串
 * @param[out] dest 目标字符串指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int safe_strdup(const char* src, char** dest)
{
    if (src == NULL || dest == NULL) {
        return SHM_INVALID_PARAM;
    }
    
    *dest = strdup(src);
    if (*dest == NULL) {
        return JSON_CONFIG_MEMORY_ERROR;
    }
    
    return SHM_SUCCESS;
}

/**
 * @brief 验证主题名称的有效性
 * @param[in] topic 主题名称
 * @return 有效返回true，无效返回false
 */
bool is_valid_topic_name(const char* topic)
{
    if (topic == NULL || strlen(topic) == 0 || strlen(topic) >= 48) {
        return false;
    }
    
    // 检查是否包含非法字符
    for (size_t i = 0; i < strlen(topic); i++) {
        char c = topic[i];
        if (!(isalnum(c) || c == '_' || c == '-' || c == '/')) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 验证端点名称的有效性
 * @param[in] name 端点名称
 * @return 有效返回true，无效返回false
 */
bool is_valid_endpoint_name(const char* name)
{
    if (name == NULL || strlen(name) == 0 || strlen(name) >= 128) {
        return false;
    }
    
    // 检查是否包含非法字符
    for (size_t i = 0; i < strlen(name); i++) {
        char c = name[i];
        if (!(isalnum(c) || c == '_' || c == '-')) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 比较函数：按topic字符串排序
 * @param[in] a 第一个元素指针
 * @param[in] b 第二个元素指针
 * @return 比较结果
 */
int cmp_by_topic(const void* a, const void* b)
{
    const json_config_center_t* pa = (const json_config_center_t*)a;
    const json_config_center_t* pb = (const json_config_center_t*)b;
    return strcmp(pa->topic, pb->topic);
}

/**
 * @brief 检查订阅者是否已存在
 * @param[in] subers 订阅者列表
 * @param[in] name 订阅者名称
 * @return 存在返回true，不存在返回false
 */
int json_config_suber_exists(const vec_json_config_suber_t* subers, const char* name)
{
    if (subers == NULL || name == NULL) {
        return false;
    }
    
    for (int i = 0; i < subers->length; i++) {
        if (strcmp(subers->data[i].name, name) == 0) {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 检查发布者是否已存在
 * @param[in] pubers 发布者列表
 * @param[in] name 发布者名称
 * @return 存在返回true，不存在返回false
 */
int json_config_puber_exists(const vec_json_config_puber_t* pubers, const char* name)
{
    if (pubers == NULL || name == NULL) {
        return false;
    }
    
    for (int i = 0; i < pubers->length; i++) {
        if (strcmp(pubers->data[i].name, name) == 0) {
            return true;
        }
    }
    
    return false;
}

/* ========================= 生命周期管理实现 ========================= */

/**
 * @brief 初始化订阅者对象
 * @param[in,out] obj 订阅者对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int json_config_suber_init(json_config_suber_t* obj)
{
    if (obj == NULL) {
        return SHM_INVALID_PARAM;
    }
    
    obj->name = NULL;
    return SHM_SUCCESS;
}

/**
 * @brief 初始化发布者对象
 * @param[in,out] obj 发布者对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int json_config_puber_init(json_config_puber_t* obj)
{
    if (obj == NULL) {
        return SHM_INVALID_PARAM;
    }
    
    obj->name = NULL;
    return SHM_SUCCESS;
}

/**
 * @brief 初始化主题配置中心对象
 * @param[in,out] obj 主题配置中心对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int json_config_center_init(json_config_center_t* obj)
{
    if (obj == NULL) {
        return SHM_INVALID_PARAM;
    }
    
    vec_init(&obj->subers);
    vec_init(&obj->pubers);
    obj->len = 0;
    obj->topic = NULL;
    obj->topic_priority = 0;
    obj->msg_size = 0;
    
    return SHM_SUCCESS;
}

/**
 * @brief 初始化JSON配置对象
 * @param[in,out] config 配置对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int json_config_init(json_config_t* config)
{
    if (config == NULL) {
        log_error("Invalid parameter: config is NULL");
        return SHM_INVALID_PARAM;
    }
    
    vec_init(&config->center);
    log_debug("JSON config object initialized");
    return SHM_SUCCESS;
}

/**
 * @brief 清理订阅者对象
 * @param[in,out] obj 订阅者对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int json_config_suber_deinit(json_config_suber_t* obj)
{
    if (obj == NULL) {
        return SHM_INVALID_PARAM;
    }
    
    if (obj->name != NULL) {
        free(obj->name);
        obj->name = NULL;
    }
    
    return SHM_SUCCESS;
}

/**
 * @brief 清理发布者对象
 * @param[in,out] obj 发布者对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int json_config_puber_deinit(json_config_puber_t* obj)
{
    if (obj == NULL) {
        return SHM_INVALID_PARAM;
    }
    
    if (obj->name != NULL) {
        free(obj->name);
        obj->name = NULL;
    }
    
    return SHM_SUCCESS;
}

/**
 * @brief 清理主题配置中心对象
 * @param[in,out] obj 主题配置中心对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int json_config_center_deinit(json_config_center_t* obj)
{
    if (obj == NULL) {
        return SHM_INVALID_PARAM;
    }
    
    // 清理订阅者列表
    for (int i = 0; i < obj->subers.length; i++) {
        json_config_suber_deinit(&obj->subers.data[i]);
    }
    vec_deinit(&obj->subers);
    
    // 清理发布者列表
    for (int i = 0; i < obj->pubers.length; i++) {
        json_config_puber_deinit(&obj->pubers.data[i]);
    }
    vec_deinit(&obj->pubers);
    
    // 清理主题名称
    if (obj->topic != NULL) {
        free(obj->topic);
        obj->topic = NULL;
    }
    
    return SHM_SUCCESS;
}

/**
 * @brief 清理JSON配置对象
 * @param[in,out] config 配置对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int json_config_deinit(json_config_t* config)
{
    if (config == NULL) {
        log_error("Invalid parameter: config is NULL");
        return SHM_INVALID_PARAM;
    }
    
    // 清理所有主题配置
    for (int i = 0; i < config->center.length; i++) {
        json_config_center_deinit(&config->center.data[i]);
    }
    vec_deinit(&config->center);
    
    log_debug("JSON config object deinitialized");
    return SHM_SUCCESS;
}

/* ========================= JSON序列化实现 ========================= */

/**
 * @brief 序列化订阅者对象
 * @param[in] obj 订阅者对象指针
 * @param[in] json JSON对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int json_config_suber_serialize(const json_config_suber_t* obj, cJSON* json)
{
    if (obj == NULL || json == NULL) {
        return SHM_INVALID_PARAM;
    }
    
    cJSON* name_json = cJSON_CreateString(obj->name);
    if (name_json == NULL) {
        return JSON_CONFIG_MEMORY_ERROR;
    }
    
    cJSON_AddItemToObject(json, "name", name_json);
    return SHM_SUCCESS;
}

/**
 * @brief 序列化发布者对象
 * @param[in] obj 发布者对象指针
 * @param[in] json JSON对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int json_config_puber_serialize(const json_config_puber_t* obj, cJSON* json)
{
    if (obj == NULL || json == NULL) {
        return SHM_INVALID_PARAM;
    }
    
    cJSON* name_json = cJSON_CreateString(obj->name);
    if (name_json == NULL) {
        return JSON_CONFIG_MEMORY_ERROR;
    }
    
    cJSON_AddItemToObject(json, "name", name_json);
    return SHM_SUCCESS;
}

/**
 * @brief 序列化主题配置中心对象
 * @param[in] obj 主题配置中心对象指针
 * @param[in] json JSON对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int json_config_center_serialize(const json_config_center_t* obj, cJSON* json)
{
    if (obj == NULL || json == NULL) {
        return SHM_INVALID_PARAM;
    }
    
    int result = SHM_SUCCESS;
    
    // 序列化订阅者列表
    cJSON* subers_array = cJSON_CreateArray();
    if (subers_array == NULL) {
        return JSON_CONFIG_MEMORY_ERROR;
    }
    
    for (int i = 0; i < obj->subers.length; i++) {
        cJSON* suber_json = cJSON_CreateObject();
        if (suber_json == NULL) {
            cJSON_Delete(subers_array);
            return JSON_CONFIG_MEMORY_ERROR;
        }
        
        result = json_config_suber_serialize(&obj->subers.data[i], suber_json);
        if (result != SHM_SUCCESS) {
            cJSON_Delete(suber_json);
            cJSON_Delete(subers_array);
            return result;
        }
        
        cJSON_AddItemToArray(subers_array, suber_json);
    }
    cJSON_AddItemToObject(json, "subers", subers_array);
    
    // 序列化发布者列表
    cJSON* pubers_array = cJSON_CreateArray();
    if (pubers_array == NULL) {
        return JSON_CONFIG_MEMORY_ERROR;
    }
    
    for (int i = 0; i < obj->pubers.length; i++) {
        cJSON* puber_json = cJSON_CreateObject();
        if (puber_json == NULL) {
            cJSON_Delete(pubers_array);
            return JSON_CONFIG_MEMORY_ERROR;
        }
        
        result = json_config_puber_serialize(&obj->pubers.data[i], puber_json);
        if (result != SHM_SUCCESS) {
            cJSON_Delete(puber_json);
            cJSON_Delete(pubers_array);
            return result;
        }
        
        cJSON_AddItemToArray(pubers_array, puber_json);
    }
    cJSON_AddItemToObject(json, "pubers", pubers_array);
    
    // 序列化基本字段
    cJSON_AddItemToObject(json, "len", cJSON_CreateNumber(obj->len));
    cJSON_AddItemToObject(json, "topic", cJSON_CreateString(obj->topic));
    cJSON_AddItemToObject(json, "topic_priority", cJSON_CreateNumber(obj->topic_priority));
    cJSON_AddItemToObject(json, "msg_size", cJSON_CreateNumber(obj->msg_size));
    
    return SHM_SUCCESS;
}

/**
 * @brief 序列化配置对象
 * @param[in] config 配置对象指针
 * @param[in] json JSON对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int json_config_serialize(const json_config_t* config, cJSON* json)
{
    if (config == NULL || json == NULL) {
        return SHM_INVALID_PARAM;
    }
    
    int result = SHM_SUCCESS;
    
    // 序列化主题配置列表
    cJSON* center_array = cJSON_CreateArray();
    if (center_array == NULL) {
        return JSON_CONFIG_MEMORY_ERROR;
    }
    
    for (int i = 0; i < config->center.length; i++) {
        cJSON* center_json = cJSON_CreateObject();
        if (center_json == NULL) {
            cJSON_Delete(center_array);
            return JSON_CONFIG_MEMORY_ERROR;
        }
        
        result = json_config_center_serialize(&config->center.data[i], center_json);
        if (result != SHM_SUCCESS) {
            cJSON_Delete(center_json);
            cJSON_Delete(center_array);
            return result;
        }
        
        cJSON_AddItemToArray(center_array, center_json);
    }
    cJSON_AddItemToObject(json, "center", center_array);
    
    return SHM_SUCCESS;
}

/**
 * @brief 将配置对象序列化为JSON字符串
 * @param[in] config 配置对象指针
 * @return 成功返回JSON字符串指针（需调用者释放），失败返回NULL
 */
char* json_config_to_str(const json_config_t* config)
{
    if (config == NULL) {
        log_error("Invalid parameter: config is NULL");
        return NULL;
    }
    
    cJSON* json = cJSON_CreateObject();
    if (json == NULL) {
        log_error("Failed to create JSON object");
        return NULL;
    }
    
    int result = json_config_serialize(config, json);
    if (result != SHM_SUCCESS) {
        log_error("Failed to serialize config, result=%d", result);
        cJSON_Delete(json);
        return NULL;
    }
    
    char* json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    
    if (json_str == NULL) {
        log_error("Failed to print JSON string");
        return NULL;
    }
    
    log_debug("Config serialized to JSON string, length=%zu", strlen(json_str));
    return json_str;
}

/**
 * @brief 将配置对象序列化到JSON文件
 * @param[in] config 配置对象指针
 * @param[in] file_path 输出文件路径
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int json_config_to_file(const json_config_t* config, const char* file_path)
{
    if (config == NULL || file_path == NULL) {
        log_error("Invalid parameters: config=%p, file_path=%p", config, file_path);
        return SHM_INVALID_PARAM;
    }
    
    char* json_str = json_config_to_str(config);
    if (json_str == NULL) {
        log_error("Failed to serialize config to string");
        return JSON_CONFIG_MEMORY_ERROR;
    }
    
    FILE* file = fopen(file_path, "w");
    if (file == NULL) {
        log_error("Failed to open file: %s, errno: %d", file_path, errno);
        free(json_str);
        return SHM_FILE_ERROR;
    }
    
    size_t written = fwrite(json_str, 1, strlen(json_str), file);
    fclose(file);
    
    if (written != strlen(json_str)) {
        log_error("Failed to write complete JSON to file: %s, written: %lu, json_str len: %lu, %s", file_path, written, strlen(json_str), json_str);
        return SHM_FILE_ERROR;
    }

    free(json_str);
    
    log_info("Config saved to file: %s", file_path);
    return SHM_SUCCESS;
}

/* ========================= JSON反序列化实现 ========================= */

/**
 * @brief 反序列化订阅者对象
 * @param[in] json JSON对象指针
 * @param[out] obj 订阅者对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int json_config_suber_deserialize(cJSON* json, json_config_suber_t* obj)
{
    if (json == NULL || obj == NULL) {
        return SHM_INVALID_PARAM;
    }
    
    cJSON* name_json = cJSON_GetObjectItemCaseSensitive(json, "name");
    if (name_json == NULL || !cJSON_IsString(name_json)) {
        log_error("Missing or invalid 'name' field in subscriber config");
        return JSON_CONFIG_INVALID_JSON;
    }
    
    return safe_strdup(name_json->valuestring, &obj->name);
}

/**
 * @brief 反序列化发布者对象
 * @param[in] json JSON对象指针
 * @param[out] obj 发布者对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int json_config_puber_deserialize(cJSON* json, json_config_puber_t* obj)
{
    if (json == NULL || obj == NULL) {
        return SHM_INVALID_PARAM;
    }
    
    cJSON* name_json = cJSON_GetObjectItemCaseSensitive(json, "name");
    if (name_json == NULL || !cJSON_IsString(name_json)) {
        log_error("Missing or invalid 'name' field in publisher config");
        return JSON_CONFIG_INVALID_JSON;
    }
    
    return safe_strdup(name_json->valuestring, &obj->name);
}

/**
 * @brief 反序列化主题配置中心对象
 * @param[in] json JSON对象指针
 * @param[out] obj 主题配置中心对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int json_config_center_deserialize(cJSON* json, json_config_center_t* obj)
{
    if (json == NULL || obj == NULL) {
        return SHM_INVALID_PARAM;
    }
    
    int result = SHM_SUCCESS;
    
    // 反序列化订阅者列表
    cJSON* subers_array = cJSON_GetObjectItemCaseSensitive(json, "subers");
    if (subers_array != NULL && cJSON_IsArray(subers_array)) {
        cJSON* suber_json;
        cJSON_ArrayForEach(suber_json, subers_array) {
            json_config_suber_t suber;
            result = json_config_suber_init(&suber);
            if (result != SHM_SUCCESS) {
                log_error("Failed to initialize subscriber");
                return result;
            }
            
            result = json_config_suber_deserialize(suber_json, &suber);
            if (result != SHM_SUCCESS) {
                json_config_suber_deinit(&suber);
                log_error("Failed to deserialize subscriber");
                return result;
            }
            
            vec_push(&obj->subers, suber);
        }
    } else {
        log_warn("Missing or invalid 'subers' field in topic config");
    }
    
    // 反序列化发布者列表
    cJSON* pubers_array = cJSON_GetObjectItemCaseSensitive(json, "pubers");
    if (pubers_array != NULL && cJSON_IsArray(pubers_array)) {
        cJSON* puber_json;
        cJSON_ArrayForEach(puber_json, pubers_array) {
            json_config_puber_t puber;
            result = json_config_puber_init(&puber);
            if (result != SHM_SUCCESS) {
                log_error("Failed to initialize publisher");
                return result;
            }
            
            result = json_config_puber_deserialize(puber_json, &puber);
            if (result != SHM_SUCCESS) {
                json_config_puber_deinit(&puber);
                log_error("Failed to deserialize publisher");
                return result;
            }
            
            vec_push(&obj->pubers, puber);
        }
    } else {
        log_warn("Missing or invalid 'pubers' field in topic config");
    }
    
    // 反序列化基本字段
    cJSON* len_json = cJSON_GetObjectItemCaseSensitive(json, "len");
    if (len_json != NULL && cJSON_IsNumber(len_json)) {
        obj->len = (int)len_json->valuedouble;
    } else {
        log_error("Missing or invalid 'len' field in topic config");
        return JSON_CONFIG_INVALID_JSON;
    }
    
    cJSON* topic_json = cJSON_GetObjectItemCaseSensitive(json, "topic");
    if (topic_json != NULL && cJSON_IsString(topic_json)) {
        result = safe_strdup(topic_json->valuestring, &obj->topic);
        if (result != SHM_SUCCESS) {
            log_error("Failed to copy topic name");
            return result;
        }
    } else {
        log_error("Missing or invalid 'topic' field in topic config");
        return JSON_CONFIG_INVALID_JSON;
    }
    
    cJSON* priority_json = cJSON_GetObjectItemCaseSensitive(json, "topic_priority");
    if (priority_json != NULL && cJSON_IsNumber(priority_json)) {
        obj->topic_priority = (int)priority_json->valuedouble;
    } else {
        log_warn("Missing or invalid 'topic_priority' field, using default 0");
        obj->topic_priority = 0;
    }
    
    cJSON* msg_size_json = cJSON_GetObjectItemCaseSensitive(json, "msg_size");
    if (msg_size_json != NULL && cJSON_IsNumber(msg_size_json)) {
        obj->msg_size = (int)msg_size_json->valuedouble;
    } else {
        log_error("Missing or invalid 'msg_size' field in topic config");
        return JSON_CONFIG_INVALID_JSON;
    }
    
    return SHM_SUCCESS;
}

/**
 * @brief 反序列化配置对象
 * @param[in] json JSON对象指针
 * @param[out] config 配置对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int json_config_deserialize(cJSON* json, json_config_t* config)
{
    if (json == NULL || config == NULL) {
        return SHM_INVALID_PARAM;
    }
    
    int result = SHM_SUCCESS;
    
    // 反序列化主题配置列表
    cJSON* center_array = cJSON_GetObjectItemCaseSensitive(json, "center");
    if (center_array != NULL && cJSON_IsArray(center_array)) {
        cJSON* center_json;
        cJSON_ArrayForEach(center_json, center_array) {
            json_config_center_t center;
            result = json_config_center_init(&center);
            if (result != SHM_SUCCESS) {
                log_error("Failed to initialize topic config center");
                return result;
            }
            
            result = json_config_center_deserialize(center_json, &center);
            if (result != SHM_SUCCESS) {
                json_config_center_deinit(&center);
                log_error("Failed to deserialize topic config center");
                return result;
            }
            
            vec_push(&config->center, center);
        }
    } else {
        log_error("Missing or invalid 'center' field in config");
        return JSON_CONFIG_INVALID_JSON;
    }
    
    return SHM_SUCCESS;
}

/**
 * @brief 从JSON字符串解析配置
 * @param[in,out] config 配置对象指针
 * @param[in] json_str JSON字符串
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int json_config_from_str(json_config_t* config, const char* json_str)
{
    if (config == NULL || json_str == NULL) {
        log_error("Invalid parameters: config=%p, json_str=%p", config, json_str);
        return SHM_INVALID_PARAM;
    }
    
    cJSON* json = cJSON_Parse(json_str);
    if (json == NULL) {
        log_error("Failed to parse JSON string");
        return JSON_CONFIG_INVALID_JSON;
    }
    
    int result = json_config_deserialize(json, config);
    cJSON_Delete(json);
    
    if (result == SHM_SUCCESS) {
        log_debug("Config parsed from JSON string, topics=%d", config->center.length);
    } else {
        log_error("Failed to parse config from JSON string, result=%d", result);
    }
    
    return result;
}

/**
 * @brief 从JSON文件解析配置
 * @param[in,out] config 配置对象指针
 * @param[in] file_path JSON文件路径
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int json_config_from_file(json_config_t* config, const char* file_path)
{
    if (config == NULL || file_path == NULL) {
        log_error("Invalid parameters: config=%p, file_path=%p", config, file_path);
        return SHM_INVALID_PARAM;
    }
    
    FILE* file = fopen(file_path, "r");
    if (file == NULL) {
        log_error("Failed to open file: %s", file_path);
        return SHM_FILE_ERROR;
    }
    
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0) {
        log_error("Invalid file size: %ld", file_size);
        fclose(file);
        return SHM_FILE_ERROR;
    }
    
    // 读取文件内容
    char* json_str = (char*)malloc(file_size + 1);
    if (json_str == NULL) {
        log_error("Failed to allocate memory for file content");
        fclose(file);
        return JSON_CONFIG_MEMORY_ERROR;
    }
    
    size_t bytes_read = fread(json_str, 1, file_size, file);
    fclose(file);
    
    if (bytes_read != (size_t)file_size) {
        log_error("Failed to read complete file: %s", file_path);
        free(json_str);
        return SHM_FILE_ERROR;
    }
    
    json_str[file_size] = '\0';
    
    int result = json_config_from_str(config, json_str);
    free(json_str);
    
    if (result == SHM_SUCCESS) {
        log_info("Config loaded from file: %s", file_path);
    }
    
    return result;
}

/* ========================= 配置合并功能实现 ========================= */

/**
 * @brief 合并主题配置中心对象
 * @param[in,out] dest 目标主题配置
 * @param[in] src 源主题配置
 * @param[in] strategy 合并策略
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int json_config_center_merge(json_config_center_t* dest, const json_config_center_t* src, merge_strategy_t strategy)
{
    if (dest == NULL || src == NULL) {
        return SHM_INVALID_PARAM;
    }
    
    // 检查主题名称是否相同
    if (strcmp(dest->topic, src->topic) != 0) {
        log_error("Cannot merge different topics: %s vs %s", dest->topic, src->topic);
        return JSON_CONFIG_MERGE_CONFLICT;
    }
    
    switch (strategy) {
        case MERGE_STRATEGY_OVERWRITE:
            // 覆盖模式：直接替换所有字段
            json_config_center_deinit(dest);
            return json_config_center_copy(src, dest);
            
        case MERGE_STRATEGY_APPEND:
            // 追加模式：合并订阅者和发布者列表
            for (int i = 0; i < src->subers.length; i++) {
                if (!json_config_suber_exists(&dest->subers, src->subers.data[i].name)) {
                    json_config_suber_t suber;
                    json_config_suber_init(&suber);
                    safe_strdup(src->subers.data[i].name, &suber.name);
                    vec_push(&dest->subers, suber);
                }
            }
            
            for (int i = 0; i < src->pubers.length; i++) {
                if (!json_config_puber_exists(&dest->pubers, src->pubers.data[i].name)) {
                    json_config_puber_t puber;
                    json_config_puber_init(&puber);
                    safe_strdup(src->pubers.data[i].name, &puber.name);
                    vec_push(&dest->pubers, puber);
                }
            }
            return SHM_SUCCESS;
            
        case MERGE_STRATEGY_MERGE:
            // 智能合并：检查参数冲突
            if (dest->len != src->len || 
                dest->topic_priority != src->topic_priority ||
                dest->msg_size != src->msg_size) {
                log_error("Topic %s has conflicting attributes", dest->topic);
                return JSON_CONFIG_MERGE_CONFLICT;
            }
            
            // 合并订阅者和发布者（去重）
            for (int i = 0; i < src->subers.length; i++) {
                if (!json_config_suber_exists(&dest->subers, src->subers.data[i].name)) {
                    json_config_suber_t suber;
                    json_config_suber_init(&suber);
                    safe_strdup(src->subers.data[i].name, &suber.name);
                    vec_push(&dest->subers, suber);
                }
            }
            
            for (int i = 0; i < src->pubers.length; i++) {
                if (!json_config_puber_exists(&dest->pubers, src->pubers.data[i].name)) {
                    json_config_puber_t puber;
                    json_config_puber_init(&puber);
                    safe_strdup(src->pubers.data[i].name, &puber.name);
                    vec_push(&dest->pubers, puber);
                }
            }
            return SHM_SUCCESS;
            
        default:
            log_error("Unsupported merge strategy: %d", strategy);
            return SHM_INVALID_PARAM;
    }
}

/**
 * @brief 合并两个配置对象
 * @param[in,out] dest 目标配置对象
 * @param[in] src 源配置对象
 * @param[in] strategy 合并策略
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 * @pre dest != NULL, src != NULL
 * @post 目标配置对象包含合并后的配置信息
 */
int json_config_merge(json_config_t* dest, const json_config_t* src, merge_strategy_t strategy)
{
    if (dest == NULL || src == NULL) {
        log_error("Invalid parameters: dest=%p, src=%p", dest, src);
        return SHM_INVALID_PARAM;
    }
    
    int result = SHM_SUCCESS;
    
    // 遍历源配置的所有主题
    for (int j = 0; j < src->center.length; j++) {
        const json_config_center_t* src_center = &src->center.data[j];
        
        // 在目标配置中查找相同主题
        bool found = false;
        for (int i = 0; i < dest->center.length; i++) {
            json_config_center_t* dest_center = &dest->center.data[i];
            
            if (strcmp(dest_center->topic, src_center->topic) == 0) {
                // 找到相同主题，进行合并
                result = json_config_center_merge(dest_center, src_center, strategy);
                if (result != SHM_SUCCESS) {
                    log_error("Failed to merge topic: %s, result=%d", src_center->topic, result);
                    return result;
                }
                found = true;
                break;
            }
        }
        
        if (!found) {
            // 新主题，直接复制到目标配置
            json_config_center_t new_center;
            result = json_config_center_init(&new_center);
            if (result != SHM_SUCCESS) {
                return result;
            }
            
            result = json_config_center_copy(src_center, &new_center);
            if (result != SHM_SUCCESS) {
                json_config_center_deinit(&new_center);
                return result;
            }
            
            vec_push(&dest->center, new_center);
        }
    }
    
    log_info("Config merged successfully, topics=%d", dest->center.length);
    return SHM_SUCCESS;
}

/**
 * @brief 从多个JSON文件合并配置
 * @param[in,out] config 配置对象指针
 * @param[in] filenames 文件路径数组
 * @param[in] count 文件数量
 * @param[in] strategy 合并策略
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int json_config_merge_files(json_config_t* config, const char** filenames, int count, merge_strategy_t strategy)
{
    if (config == NULL || filenames == NULL || count <= 0) {
        log_error("Invalid parameters: config=%p, filenames=%p, count=%d", config, filenames, count);
        return SHM_INVALID_PARAM;
    }
    
    int result = SHM_SUCCESS;
    
    // 初始化配置对象
    result = json_config_init(config);
    if (result != SHM_SUCCESS) {
        return result;
    }
    
    // 逐个加载并合并配置文件
    for (int i = 0; i < count; i++) {
        if (filenames[i] == NULL) {
            log_error("Invalid filename at index %d", i);
            json_config_deinit(config);
            return SHM_INVALID_PARAM;
        }
        
        json_config_t temp_config;
        result = json_config_init(&temp_config);
        if (result != SHM_SUCCESS) {
            json_config_deinit(config);
            return result;
        }
        
        result = json_config_from_file(&temp_config, filenames[i]);
        if (result != SHM_SUCCESS) {
            log_error("Failed to load config file: %s", filenames[i]);
            json_config_deinit(&temp_config);
            json_config_deinit(config);
            return result;
        }
        
        // 依次把文件合并进来
        result = json_config_merge(config, &temp_config, strategy);
        
        json_config_deinit(&temp_config);
        
        if (result != SHM_SUCCESS) {
            log_error("Failed to merge config file: %s", filenames[i]);
            json_config_deinit(config);
            return result;
        }
    }
    
    log_info("Successfully merged %d config files", count);
    return SHM_SUCCESS;
}

/**
 * @brief 从多个JSON字符串合并配置
 * @param[in,out] config 配置对象指针
 * @param[in] json_strs JSON字符串数组
 * @param[in] count 字符串数量
 * @param[in] strategy 合并策略
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int json_config_merge_strs(json_config_t* config, const char** json_strs, int count, merge_strategy_t strategy)
{
    if (config == NULL || json_strs == NULL || count <= 0) {
        log_error("Invalid parameters: config=%p, json_strs=%p, count=%d", config, json_strs, count);
        return SHM_INVALID_PARAM;
    }
    
    int result = SHM_SUCCESS;
    
    // 初始化配置对象
    result = json_config_init(config);
    if (result != SHM_SUCCESS) {
        return result;
    }
    
    // 逐个解析并合并配置字符串
    for (int i = 0; i < count; i++) {
        if (json_strs[i] == NULL) {
            log_error("Invalid JSON string at index %d", i);
            json_config_deinit(config);
            return SHM_INVALID_PARAM;
        }
        
        json_config_t temp_config;
        result = json_config_init(&temp_config);
        if (result != SHM_SUCCESS) {
            json_config_deinit(config);
            return result;
        }
        
        result = json_config_from_str(&temp_config, json_strs[i]);
        if (result != SHM_SUCCESS) {
            log_error("Failed to parse JSON string at index %d", i);
            json_config_deinit(&temp_config);
            json_config_deinit(config);
            return result;
        }
        
        
        // 后续配置字符串进行合并
        result = json_config_merge(config, &temp_config, strategy);
        
        
        json_config_deinit(&temp_config);
        
        if (result != SHM_SUCCESS) {
            log_error("Failed to merge JSON string at index %d", i);
            json_config_deinit(config);
            return result;
        }
    }
    
    log_info("Successfully merged %d JSON strings", count);
    return SHM_SUCCESS;
}

/* ========================= 配置验证功能实现 ========================= */

/**
 * @brief 验证主题配置中心对象的有效性
 * @param[in] center 主题配置中心对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int json_config_center_validate(const json_config_center_t* center)
{
    if (center == NULL) {
        return SHM_INVALID_PARAM;
    }
    
    // 验证主题名称
    if (!is_valid_topic_name(center->topic)) {
        log_error("Invalid topic name: %s", center->topic ? center->topic : "NULL");
        return JSON_CONFIG_VALIDATION_FAIL;
    }
    
    // 验证参数范围
    if (center->len <= 0 || center->len > 10000) {
        log_error("Invalid queue length: %d for topic: %s", center->len, center->topic);
        return JSON_CONFIG_VALIDATION_FAIL;
    }
    
    if (center->msg_size <= 0 || center->msg_size > 1024 * 1024) {
        log_error("Invalid message size: %d for topic: %s", center->msg_size, center->topic);
        return JSON_CONFIG_VALIDATION_FAIL;
    }
    
    if (center->topic_priority < 0 || center->topic_priority > 1000) {
        log_error("Invalid topic priority: %d for topic: %s", center->topic_priority, center->topic);
        return JSON_CONFIG_VALIDATION_FAIL;
    }
    
    // 验证订阅者
    for (int i = 0; i < center->subers.length; i++) {
        if (!is_valid_endpoint_name(center->subers.data[i].name)) {
            log_error("Invalid subscriber name: %s for topic: %s", 
                     center->subers.data[i].name ? center->subers.data[i].name : "NULL", center->topic);
            return JSON_CONFIG_VALIDATION_FAIL;
        }
    }
    
    // 验证发布者
    for (int i = 0; i < center->pubers.length; i++) {
        if (!is_valid_endpoint_name(center->pubers.data[i].name)) {
            log_error("Invalid publisher name: %s for topic: %s", 
                     center->pubers.data[i].name ? center->pubers.data[i].name : "NULL", center->topic);
            return JSON_CONFIG_VALIDATION_FAIL;
        }
    }
    
    // 验证至少有一个订阅者和一个发布者
    if (center->subers.length == 0) {
        log_error("No subscribers for topic: %s", center->topic);
        return JSON_CONFIG_VALIDATION_FAIL;
    }
    
    if (center->pubers.length == 0) {
        log_error("No publishers for topic: %s", center->topic);
        return JSON_CONFIG_VALIDATION_FAIL;
    }
    
    return SHM_SUCCESS;
}

/**
 * @brief 验证配置对象的有效性
 * @param[in] config 配置对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int json_config_validate(const json_config_t* config)
{
    if (config == NULL) {
        log_error("Invalid parameter: config is NULL");
        return SHM_INVALID_PARAM;
    }
    
    // 验证至少有一个主题配置
    if (config->center.length == 0) {
        log_error("No topic configurations found");
        return JSON_CONFIG_VALIDATION_FAIL;
    }
    
    // 验证所有主题配置
    for (int i = 0; i < config->center.length; i++) {
        int result = json_config_center_validate(&config->center.data[i]);
        if (result != SHM_SUCCESS) {
            log_error("Topic config validation failed at index %d", i);
            return result;
        }
    }
    
    // 检查重复主题
    for (int i = 0; i < config->center.length; i++) {
        for (int j = i + 1; j < config->center.length; j++) {
            if (strcmp(config->center.data[i].topic, config->center.data[j].topic) == 0) {
                log_error("Duplicate topic found: %s", config->center.data[i].topic);
                return JSON_CONFIG_VALIDATION_FAIL;
            }
        }
    }
    
    log_debug("Config validation passed, topics=%d", config->center.length);
    return SHM_SUCCESS;
}

/**
 * @brief 获取配置中的主题数量
 * @param[in] config 配置对象指针
 * @return 主题数量，如果配置无效返回-1
 */
int json_config_get_topic_count(const json_config_t* config)
{
    if (config == NULL) {
        log_error("Invalid parameter: config is NULL");
        return -1;
    }
    
    return config->center.length;
}

/**
 * @brief 检查指定主题是否存在
 * @param[in] config 配置对象指针
 * @param[in] topic_name 主题名称
 * @return 存在返回true，不存在返回false
 */
bool json_config_has_topic(const json_config_t* config, const char* topic_name)
{
    if (config == NULL || topic_name == NULL) {
        log_error("Invalid parameters: config=%p, topic_name=%p", config, topic_name);
        return false;
    }
    
    for (int i = 0; i < config->center.length; i++) {
        if (strcmp(config->center.data[i].topic, topic_name) == 0) {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 获取指定主题的配置
 * @param[in] config 配置对象指针
 * @param[in] topic_name 主题名称
 * @return 成功返回主题配置指针，失败返回NULL
 */
const json_config_center_t* json_config_get_topic(const json_config_t* config, const char* topic_name)
{
    if (config == NULL || topic_name == NULL) {
        log_error("Invalid parameters: config=%p, topic_name=%p", config, topic_name);
        return NULL;
    }
    
    for (int i = 0; i < config->center.length; i++) {
        if (strcmp(config->center.data[i].topic, topic_name) == 0) {
            return &config->center.data[i];
        }
    }
    
    return NULL;
}

/* ========================= 工具函数实现 ========================= */

/**
 * @brief 复制主题配置中心对象
 * @param[in] src 源主题配置
 * @param[out] dest 目标主题配置
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int json_config_center_copy(const json_config_center_t* src, json_config_center_t* dest)
{
    if (src == NULL || dest == NULL) {
        return SHM_INVALID_PARAM;
    }
    
    int result = SHM_SUCCESS;
    
    // 初始化目标对象
    result = json_config_center_init(dest);
    if (result != SHM_SUCCESS) {
        return result;
    }
    
    // 复制基本字段
    dest->len = src->len;
    dest->topic_priority = src->topic_priority;
    dest->msg_size = src->msg_size;
    
    result = safe_strdup(src->topic, &dest->topic);
    if (result != SHM_SUCCESS) {
        json_config_center_deinit(dest);
        return result;
    }
    
    // 复制订阅者列表
    for (int i = 0; i < src->subers.length; i++) {
        json_config_suber_t suber;
        result = json_config_suber_init(&suber);
        if (result != SHM_SUCCESS) {
            json_config_center_deinit(dest);
            return result;
        }
        
        result = safe_strdup(src->subers.data[i].name, &suber.name);
        if (result != SHM_SUCCESS) {
            json_config_suber_deinit(&suber);
            json_config_center_deinit(dest);
            return result;
        }
        
        vec_push(&dest->subers, suber);
    }
    
    // 复制发布者列表
    for (int i = 0; i < src->pubers.length; i++) {
        json_config_puber_t puber;
        result = json_config_puber_init(&puber);
        if (result != SHM_SUCCESS) {
            json_config_center_deinit(dest);
            return result;
        }
        
        result = safe_strdup(src->pubers.data[i].name, &puber.name);
        if (result != SHM_SUCCESS) {
            json_config_puber_deinit(&puber);
            json_config_center_deinit(dest);
            return result;
        }
        
        vec_push(&dest->pubers, puber);
    }
    
    return SHM_SUCCESS;
}

/**
 * @brief 比较两个主题配置是否相等
 * @param[in] center1 第一个主题配置
 * @param[in] center2 第二个主题配置
 * @return 相等返回0，不相等返回非0
 */
int json_config_center_equals(const json_config_center_t* center1, const json_config_center_t* center2)
{
    if (center1 == NULL || center2 == NULL) {
        return -1;
    }
    
    // 比较基本字段
    if (center1->len != center2->len || 
        center1->topic_priority != center2->topic_priority ||
        center1->msg_size != center2->msg_size ||
        strcmp(center1->topic, center2->topic) != 0) {
        return -1;
    }
    
    // 比较订阅者数量
    if (center1->subers.length != center2->subers.length) {
        return -1;
    }
    
    // 比较发布者数量
    if (center1->pubers.length != center2->pubers.length) {
        return -1;
    }
    
    // 比较订阅者（顺序无关）
    for (int i = 0; i < center1->subers.length; i++) {
        bool found = false;
        for (int j = 0; j < center2->subers.length; j++) {
            if (strcmp(center1->subers.data[i].name, center2->subers.data[j].name) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return -1;
        }
    }
    
    // 比较发布者（顺序无关）
    for (int i = 0; i < center1->pubers.length; i++) {
        bool found = false;
        for (int j = 0; j < center2->pubers.length; j++) {
            if (strcmp(center1->pubers.data[i].name, center2->pubers.data[j].name) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return -1;
        }
    }
    
    return 0;
}

/**
 * @brief 复制配置对象
 * @param[in] src 源配置对象指针
 * @param[out] dest 目标配置对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int json_config_copy(const json_config_t* src, json_config_t* dest)
{
    if (src == NULL || dest == NULL) {
        log_error("Invalid parameters: src=%p, dest=%p", src, dest);
        return SHM_INVALID_PARAM;
    }
    
    int result = SHM_SUCCESS;
    
    // 初始化目标对象
    result = json_config_init(dest);
    if (result != SHM_SUCCESS) {
        return result;
    }
    
    // 复制所有主题配置
    for (int i = 0; i < src->center.length; i++) {
        json_config_center_t center;
        result = json_config_center_copy(&src->center.data[i], &center);
        if (result != SHM_SUCCESS) {
            json_config_deinit(dest);
            return result;
        }
        
        vec_push(&dest->center, center);
    }
    
    return SHM_SUCCESS;
}

/**
 * @brief 比较两个配置对象是否相等
 * @param[in] config1 第一个配置对象指针
 * @param[in] config2 第二个配置对象指针
 * @return 相等返回true，不相等返回false
 */
bool json_config_equals(const json_config_t* config1, const json_config_t* config2)
{
    if (config1 == NULL || config2 == NULL) {
        return false;
    }
    
    // 比较主题数量
    if (config1->center.length != config2->center.length) {
        return false;
    }
    
    // 比较所有主题配置
    for (int i = 0; i < config1->center.length; i++) {
        bool found = false;
        for (int j = 0; j < config2->center.length; j++) {
            if (json_config_center_equals(&config1->center.data[i], &config2->center.data[j]) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 获取合并策略的字符串描述
 * @param[in] strategy 合并策略
 * @return 策略描述字符串
 */
const char* json_config_merge_strategy_to_str(merge_strategy_t strategy)
{
    switch (strategy) {
        case MERGE_STRATEGY_OVERWRITE:
            return "OVERWRITE";
        case MERGE_STRATEGY_APPEND:
            return "APPEND";
        case MERGE_STRATEGY_MERGE:
            return "MERGE";
        case MERGE_STRATEGY_CUSTOM:
            return "CUSTOM";
        default:
            return "UNKNOWN";
    }
}
