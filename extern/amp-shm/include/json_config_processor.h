/*
 * @file json_config_processor.h
 * @brief JSON配置处理模块接口
 * @copyright 2023 AMP-SHM Project, MIT License
 * 
 * 提供统一的JSON配置序列化、反序列化、合并、验证功能，支持相同主题项的智能合并。
 * 线程安全：配置对象需要外部同步，多线程环境下需要加锁保护。
 */

#ifndef AMP_SHM_JSON_CONFIG_PROCESSOR_H_
#define AMP_SHM_JSON_CONFIG_PROCESSOR_H_

#include <stdint.h>
#include <stdbool.h>
#include "cJSON.h"
#include "vec.h"
#include "shm_errors.h"

#ifdef __cplusplus
extern "C"
{
#endif

// 前置声明避免循环依赖
typedef struct json_config json_config_t;
typedef struct json_config_center json_config_center_t;
typedef struct json_config_suber json_config_suber_t;
typedef struct json_config_puber json_config_puber_t;

// 向量类型定义
typedef vec_t(json_config_center_t) vec_json_config_center_t;
typedef vec_t(json_config_suber_t) vec_json_config_suber_t;
typedef vec_t(json_config_puber_t) vec_json_config_puber_t;

/**
 * @brief 合并策略枚举
 */
typedef enum {
    MERGE_STRATEGY_OVERWRITE = 0,  /**< 覆盖模式：后配置覆盖先配置 */
    MERGE_STRATEGY_APPEND = 1,     /**< 追加模式：合并订阅者和发布者列表 */
    MERGE_STRATEGY_MERGE = 2,      /**< 智能合并：去重合并，参数冲突时报错 */
    MERGE_STRATEGY_CUSTOM = 3      /**< 自定义合并策略（保留） */
} merge_strategy_t;

/**
 * @brief 订阅者配置结构
 */
typedef struct json_config_suber {
    char* name;                     /**< 订阅者名称 */
} json_config_suber_t;

/**
 * @brief 发布者配置结构
 */
typedef struct json_config_puber {
    char* name;                     /**< 发布者名称 */
} json_config_puber_t;

/**
 * @brief 主题配置中心结构
 */
typedef struct json_config_center {
    vec_json_config_suber_t subers; /**< 订阅者列表 */
    vec_json_config_puber_t pubers; /**< 发布者列表 */
    int len;                        /**< 队列长度 */
    char* topic;                    /**< 主题名称 */
    int topic_priority;             /**< 主题优先级 */
    int msg_size;                   /**< 消息大小 */
} json_config_center_t;

/**
 * @brief JSON配置对象结构
 */
typedef struct json_config {
    vec_json_config_center_t center; /**< 主题配置列表 */
} json_config_t;

/* ========================= 生命周期管理 ========================= */

/**
 * @brief 初始化JSON配置对象
 * @param[in,out] config 配置对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 * @pre config != NULL
 * @post 配置对象被初始化为空状态
 */
int json_config_init(json_config_t* config);

/**
 * @brief 清理JSON配置对象
 * @param[in,out] config 配置对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 * @pre config != NULL
 * @post 配置对象被清理，所有资源被释放
 */
int json_config_deinit(json_config_t* config);

/* ========================= JSON序列化/反序列化 ========================= */

/**
 * @brief 从JSON字符串解析配置
 * @param[in,out] config 配置对象指针
 * @param[in] json_str JSON字符串
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 * @pre config != NULL, json_str != NULL
 * @post 配置对象包含解析后的配置信息
 */
int json_config_from_str(json_config_t* config, const char* json_str);

/**
 * @brief 从JSON文件解析配置
 * @param[in,out] config 配置对象指针
 * @param[in] file_path JSON文件路径
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 * @pre config != NULL, file_path != NULL
 * @post 配置对象包含解析后的配置信息
 */
int json_config_from_file(json_config_t* config, const char* file_path);

/**
 * @brief 将配置对象序列化为JSON字符串
 * @param[in] config 配置对象指针
 * @return 成功返回JSON字符串指针（需调用者释放），失败返回NULL
 * @pre config != NULL
 * @note 返回的字符串需要调用者使用free()释放
 */
char* json_config_to_str(const json_config_t* config);

/**
 * @brief 将配置对象序列化到JSON文件
 * @param[in] config 配置对象指针
 * @param[in] file_path 输出文件路径
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 * @pre config != NULL, file_path != NULL
 */
int json_config_to_file(const json_config_t* config, const char* file_path);

/* ========================= 配置合并功能 ========================= */

/**
 * @brief 合并两个配置对象
 * @param[in,out] dest 目标配置对象
 * @param[in] src 源配置对象
 * @param[in] strategy 合并策略
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 * @pre dest != NULL, src != NULL
 * @post 目标配置对象包含合并后的配置信息
 */
int json_config_merge(json_config_t* dest, const json_config_t* src, merge_strategy_t strategy);

/**
 * @brief 从多个JSON文件合并配置，合并逻辑为，以第一个文件为基准，将其他文件合并到第一个文件中
 * @param[in,out] config 配置对象指针
 * @param[in] filenames 文件路径数组
 * @param[in] count 文件数量
 * @param[in] strategy 合并策略
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 * @pre config != NULL, filenames != NULL, count > 0
 * @post 配置对象包含合并后的配置信息
 */
int json_config_merge_files(json_config_t* config, const char** filenames, int count, merge_strategy_t strategy);

/**
 * @brief 从多个JSON字符串合并配置，以第一个JSON字符串为基准，将其他文件合并到第一个文件中
 * @param[in,out] config 配置对象指针
 * @param[in] json_strs JSON字符串数组
 * @param[in] count 字符串数量
 * @param[in] strategy 合并策略
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 * @pre config != NULL, json_strs != NULL, count > 0
 * @post 配置对象包含合并后的配置信息
 */
int json_config_merge_strs(json_config_t* config, const char** json_strs, int count, merge_strategy_t strategy);

/* ========================= 配置验证功能 ========================= */

/**
 * @brief 验证配置对象的有效性
 * @param[in] config 配置对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 * @pre config != NULL
 */
int json_config_validate(const json_config_t* config);

/**
 * @brief 获取配置中的主题数量
 * @param[in] config 配置对象指针
 * @return 主题数量，如果配置无效返回-1
 * @pre config != NULL
 */
int json_config_get_topic_count(const json_config_t* config);

/**
 * @brief 检查指定主题是否存在
 * @param[in] config 配置对象指针
 * @param[in] topic_name 主题名称
 * @return 存在返回true，不存在返回false
 * @pre config != NULL, topic_name != NULL
 */
bool json_config_has_topic(const json_config_t* config, const char* topic_name);

/**
 * @brief 获取指定主题的配置
 * @param[in] config 配置对象指针
 * @param[in] topic_name 主题名称
 * @return 成功返回主题配置指针，失败返回NULL
 * @pre config != NULL, topic_name != NULL
 * @note 返回的指针指向内部数据，不要直接修改
 */
const json_config_center_t* json_config_get_topic(const json_config_t* config, const char* topic_name);

/* ========================= 工具函数 ========================= */

/**
 * @brief 复制配置对象
 * @param[in] src 源配置对象指针
 * @param[out] dest 目标配置对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 * @pre src != NULL, dest != NULL
 */
int json_config_copy(const json_config_t* src, json_config_t* dest);

/**
 * @brief 比较两个配置对象是否相等
 * @param[in] config1 第一个配置对象指针
 * @param[in] config2 第二个配置对象指针
 * @return 相等返回true，不相等返回false
 * @pre config1 != NULL, config2 != NULL
 */
bool json_config_equals(const json_config_t* config1, const json_config_t* config2);

/**
 * @brief 获取合并策略的字符串描述
 * @param[in] strategy 合并策略
 * @return 策略描述字符串
 */
const char* json_config_merge_strategy_to_str(merge_strategy_t strategy);

#ifdef __cplusplus
}
#endif

#endif  // AMP_SHM_JSON_CONFIG_PROCESSOR_H_
