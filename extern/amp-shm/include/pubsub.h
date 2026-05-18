#ifndef AMP_SHM_PUBSUB_H_
#define AMP_SHM_PUBSUB_H_

#include <stdint.h>
#include "json_config_processor.h"
#include "shm_errors.h"
#include "shm_comm.h"

// 前置声明避免循环依赖
typedef struct pubsub_config pubsub_config;

/**
 * @file pubsub.h
 * @brief 共享内存发布订阅核心接口
 * @copyright 2023 AMP-SHM Project, MIT License
 * 
 * 提供基于共享内存的高性能发布订阅功能，支持多对多通信模式。
 * 线程安全：核心消息传递无锁设计，配置管理需要外部同步。
 */

#define MAX_TOPIC_LEN 32  /**< 主题名称最大长度 */

#pragma pack(1)

/**
 * @brief 发布订阅消息头结构
 * 
 * 用于在共享内存中传递消息的头部信息，包含消息元数据和负载。
 */
typedef struct
{
    uint32_t size;                  /**< 消息负载大小（字节） */
    char topic[MAX_TOPIC_LEN];      /**< 消息主题名称 */
    uint8_t opcode;                 /**< 操作码（保留字段） */
    char payload[];                 /**< 消息负载数据（柔性数组） */
} PubSubHdr;

#pragma pack(0)

typedef struct
{
    json_config_t pcfg_json_obj;
    shm_comm_config_t shm_comm_config;
    uint32_t recv_sleep_ns;       /**< recv_msg函数的休眠延时（纳秒），默认为0 */
} pubsub_config_new_t;

/**
 * @brief 初始化发布订阅配置对象（新版本）
 * @param[in,out] pcfg 发布订阅配置对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 * @pre pcfg != NULL
 * @post 配置对象被初始化为空状态
 */
int pubsub_config_new_init(pubsub_config_new_t* pcfg);

/**
 * @brief 释放发布订阅配置对象（新版本）
 * @param[in,out] pcfg 发布订阅配置对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 * @pre pcfg != NULL
 * @post 配置对象被清理，所有资源被释放
 */
int pubsub_config_new_deinit(pubsub_config_new_t* pcfg);


/**
 * @brief 从配置文件初始化发布订阅配置
 * @param[in,out] pcfg 发布订阅配置对象指针
 * @param[in] filenames 配置文件路径数组
 * @param[in] count 配置文件数量
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 * @pre pcfg != NULL, filenames != NULL, count > 0
 * @post 配置对象被初始化，包含所有解析的配置信息
 */
int pub_init_config_files(pubsub_config_new_t* pcfg, const char** filenames, int count);

/**
 * @brief 从JSON字符串初始化发布订阅配置
 * @param[in,out] pcfg 发布订阅配置对象指针
 * @param[in] json_strs JSON字符串数组
 * @param[in] count JSON字符串数量
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 * @pre pcfg != NULL, json_strs != NULL, count > 0
 * @post 配置对象被初始化，包含所有解析的配置信息
 */
int pub_init_config_strs(pubsub_config_new_t* pcfg, const char** json_strs, int count);

/**
 * @brief 计算发布订阅所需共享内存大小
 * @return 所需共享内存大小（字节）
 * @note 此大小包括所有主题队列和端点管理所需内存
 */
int pubsub_cal_mem(pubsub_config_new_t* pcfg);

/**
 * @brief 初始化发布订阅系统
 * @param[in] is_main 是否为主进程（1=主进程，0=从进程）
 * @param[in] shm_addr 共享内存地址
 * @param[in] mem_len 共享内存大小
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 * @pre shm_addr != NULL, mem_len >= pubsub_cal_mem()返回值
 * @post 发布订阅系统初始化完成，可以开始消息传递
 */
int pubsub_init(pubsub_config_new_t* pcfg, int is_main, void* shm_addr, int mem_len);

/**
 * @brief 发布消息到指定主题
 * @param[in] pcfg 发布订阅配置对象指针
 * @param[in] puber_name 发布者名称
 * @param[in] topic 主题名称
 * @param[in] msg 消息数据指针
 * @param[in] len 消息数据长度
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 * @pre pcfg != NULL, puber_name != NULL, topic != NULL, msg != NULL, len > 0
 * @post 消息被发布到所有订阅该主题的订阅者
 */
int pub_msg(pubsub_config_new_t* pcfg, const char* puber_name, const char* topic, const void* msg, uint32_t len);

/**
 * @brief 接收消息（循环while 1，需要单独起线程运行），仅接收到消息后把消息发布到OM模块的本地发布订阅中，需通过om_config_suber设置回调实际接收消息
 * @param[in] pcfg 发布订阅配置对象指针
 * @param[in] recver_name 接收者名称
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 * @pre pcfg != NULL, recver_name != NULL
 * @note 此函数为阻塞调用，会持续接收消息直到进程退出
 */
int recv_msg(pubsub_config_new_t* pcfg, const char* recver_name);

#endif  // AMP_SHM_PUBSUB_H_
