/**
 * @file shm_comm.h
 * @brief 共享内存通信模块接口
 * @copyright 2023 AMP-SHM Project, MIT License
 * 
 * 提供基于共享内存的高性能进程间通信功能，支持多通道消息传递。
 * 包含共享内存初始化、通道管理、消息发送接收等核心功能。
 * 
 * @note 线程安全：核心消息传递采用无锁设计，配置管理需要外部同步
 * @warning 共享内存操作需要进程间协调，确保正确的初始化顺序
 */

#ifndef SHM_COMM_H
#define SHM_COMM_H

#include <stdint.h>
#include <stdbool.h>
#include "shm_queue.h"
#include "shm_endpoint.h"

/** @brief 发送者/接收者名称最大长度 */
#define SR_NAME_LEN 128
/** @brief 最大通道数量 */
#define MAX_NODE_NUM 64

/**
 * @brief 共享内存通信通道配置
 * 
 * 定义单个通信通道的配置参数，包括发送者、接收者名称和数据大小。
 */
typedef struct shm_comm_channel {
    char sender_name[SR_NAME_LEN];    /**< 发送者名称 */
    char recver_name[SR_NAME_LEN];    /**< 接收者名称 */
    uint32_t data_size;               /**< 单条消息数据大小（字节） */
    uint32_t size;                    /**< 队列容量（消息数量） */
} shm_comm_channel_t;

/**
 * @brief 共享内存通信配置
 * 
 * 包含所有通信通道的配置信息和端点表指针。
 */
typedef struct shm_comm_config {
    int channel_num;                  /**< 通道数量 */
    shm_comm_channel_t channels[MAX_NODE_NUM]; /**< 通道配置数组 */
    shm_endpoint_table_t* ep_table;   /**< 端点表指针，用于共享内存，不用释放 */
} shm_comm_config_t;

/**
 * @brief 打印共享内存通信统计信息
 * 
 * 输出所有端点的发送接收统计信息，包括字节数、错误计数等。
 */
void shm_print_stats();

/**
 * @brief 主进程共享内存初始化
 * 
 * 主进程调用此函数初始化共享内存区域，负责创建和设置共享内存结构。
 * 
 * @param[in,out] config 共享内存通信配置
 * @param[in] shm_addr 共享内存起始地址
 * @param[in] size 共享内存总大小（字节）
 * @return int 成功返回SHM_SUCCESS，失败返回错误码
 * @pre config != NULL, shm_addr != NULL, size >= 所需最小内存
 * @post 共享内存区域被初始化为可用状态，端点表指针被设置
 */
int shm_comm_init_main(shm_comm_config_t* config, void* shm_addr, uint32_t size);

/**
 * @brief 从进程共享内存初始化
 * 
 * 从进程调用此函数连接到已存在的共享内存区域。
 * 
 * @param[in,out] config 共享内存通信配置
 * @param[in] shm_addr 共享内存起始地址
 * @return int 成功返回SHM_SUCCESS，失败返回错误码
 * @pre config != NULL, shm_addr != NULL
 * @post 从进程可以访问共享内存中的端点表，端点表指针被设置
 */
int shm_comm_init_minor(shm_comm_config_t* config, void* shm_addr);

/**
 * @brief 将共享内存指针置为NULL
 * 
 * 重置内存指针。
 * 
 * @param[in,out] config 共享内存通信配置
 * @return int 成功返回SHM_SUCCESS，失败返回错误码
 * @pre config != NULL
 * @post 端点表指针被置为NULL
 */
int shm_comm_deinit(shm_comm_config_t* config);

/**
 * @brief 向配置中添加新通信通道
 * 
 * 在运行时动态添加新的通信通道配置。
 * 
 * @param[in,out] config 共享内存通信配置
 * @param[in] sender_name 发送者名称
 * @param[in] recver_name 接收者名称
 * @param[in] data_size 单条消息数据大小（字节）
 * @param[in] size 队列容量（消息数量）
 * @return int 成功返回SHM_SUCCESS，失败返回错误码
 * @pre config != NULL, sender_name != NULL, recver_name != NULL, data_size > 0, size > 0
 * @pre strlen(sender_name) < MAX_ENDPOINT_NAME
 * @pre strlen(recver_name) < MAX_ENDPOINT_NAME
 * @pre config->channel_num < MAX_NODE_NUM
 */
int shm_comm_config_add_channel(shm_comm_config_t* config, const char* sender_name, const char* recver_name, 
                              uint32_t data_size, uint32_t size);

/**
 * @brief 计算所需共享内存大小
 * 
 * 根据当前配置计算所需的共享内存总大小。
 * 
 * @param[in] config 共享内存通信配置
 * @return int 所需内存大小（字节）
 * @pre config != NULL
 * @note 计算结果包括端点表和各队列缓冲区的大小
 */
int shm_comm_cal_mem(shm_comm_config_t* config);

/**
 * @brief 加载配置到共享内存
 * 
 * 将配置中的通道信息加载到共享内存，创建对应的端点。
 * 
 * @param[in,out] config 共享内存通信配置
 * @return int 成功返回SHM_SUCCESS，失败返回错误码
 * @pre config != NULL, config->ep_table != NULL
 * @pre 共享内存已正确初始化
 */
int shm_comm_load_config(shm_comm_config_t* config);

/**
 * @brief 清除共享内存中的数据
 * 
 * 重置端点表，清除所有端点的统计信息和队列数据。
 * 
 * @param[in,out] config 共享内存通信配置
 * @return int 成功返回SHM_SUCCESS，失败返回错误码
 * @pre config != NULL, config->ep_table != NULL
 * @post 所有端点的统计信息被重置，队列被清空
 */
int shm_comm_clear(shm_comm_config_t* config);

/**
 * @brief 创建新的通信端点
 * 
 * 在共享内存中创建新的通信端点。
 * 
 * @param[in,out] config 共享内存通信配置
 * @param[in] name 端点名称
 * @param[in] data_size 单条消息数据大小（字节）
 * @param[in] size 队列容量（重要：因为内部为环形队列，所以消息容量是队列大小-1）
 * @return int 成功返回SHM_SUCCESS，失败返回错误码
 * @pre config != NULL, config->ep_table != NULL, name != NULL, data_size > 0, size > 0
 * @pre strlen(name) < MAX_ENDPOINT_NAME
 */
int shm_comm_create_endpoint_base(shm_comm_config_t* config, const char* name, uint32_t data_size, uint32_t size);

/**
 * @brief 获取指定通道的端点
 * 
 * 根据发送者和接收者名称查找对应的通信端点。
 * 
 * @param[in] config 共享内存通信配置
 * @param[in] sender 发送者名称
 * @param[in] recver 接收者名称
 * @return shm_endpoint_t* 找到返回端点指针，未找到返回NULL
 * @pre config != NULL, config->ep_table != NULL, sender != NULL, recver != NULL
 */
shm_endpoint_t* shm_comm_get_channel(shm_comm_config_t* config, const char* sender, const char* recver);

/**
 * @brief 获取端点表指针
 * 
 * 返回端点表的指针。
 * 
 * @param[in] config 共享内存通信配置
 * @return shm_endpoint_table_t* 端点表指针，如果配置无效返回NULL
 * @pre config != NULL
 */
shm_endpoint_table_t* shm_get_endpoint_table(shm_comm_config_t* config);

/**
 * @brief 发送消息到指定端点
 * 
 * 将数据发送到指定的通信端点。
 * 
 * @param[in] ep_table 端点表指针
 * @param[in] endpoint 目标端点
 * @param[in] data 要发送的数据
 * @param[in] size 数据大小（字节）
 * @return int 成功返回SHM_SUCCESS，失败返回错误码
 * @pre ep_table != NULL, endpoint != NULL, data != NULL, size > 0
 * @pre size <= endpoint->queue.data_size
 * @post 发送统计信息被更新
 */
int shm_comm_send(shm_endpoint_table_t* ep_table, shm_endpoint_t* endpoint, void* data, uint32_t size);

/**
 * @brief 分配发送消息缓冲区（零拷贝）
 * 
 * 为发送操作分配消息缓冲区，支持零拷贝发送。
 * 
 * @param[in] ep_table 端点表指针
 * @param[in] endpoint 目标端点
 * @param[in] size 需要分配的大小（字节）
 * @return shm_base_msg_t* 成功返回消息缓冲区指针，失败返回NULL
 * @pre ep_table != NULL, endpoint != NULL, size > 0
 * @pre size <= endpoint->queue.data_size
 * @note 调用者需要手动填充消息内容
 */
shm_base_msg_t* shm_comm_send_alloc(shm_endpoint_table_t* ep_table, shm_endpoint_t* endpoint, uint32_t size);

/**
 * @brief 完成零拷贝发送操作
 * 
 * 提交通过shm_comm_send_alloc分配的消息缓冲区。
 * 
 * @param[in] ep_table 端点表指针
 * @param[in] endpoint 目标端点
 * @pre ep_table != NULL, endpoint != NULL
 * @post 消息被提交到队列，统计信息被更新
 */
void shm_comm_send_alloc_end(shm_endpoint_table_t* ep_table, shm_endpoint_t* endpoint);

/**
 * @brief 从指定端点接收消息
 * 
 * 从指定的通信端点接收消息。
 * 
 * @param[in] ep_table 端点表指针
 * @param[in] endpoint 源端点
 * @param[out] buffer 接收缓冲区
 * @param[in] size 缓冲区大小（字节）
 * @return int 成功返回接收到的数据大小，失败返回错误码
 * @pre ep_table != NULL, endpoint != NULL, buffer != NULL, size > 0
 * @pre size >= endpoint->queue.data_size
 * @post 接收统计信息被更新
 */
int shm_comm_recv(shm_endpoint_table_t* ep_table, shm_endpoint_t* endpoint, void* buffer, uint32_t size);

/**
 * @brief 零拷贝接收消息
 * 
 * 以零拷贝方式接收消息，返回消息缓冲区指针。
 * 
 * @param[in] ep_table 端点表指针
 * @param[in] endpoint 源端点
 * @return shm_base_msg_t* 成功返回消息指针，失败返回NULL
 * @pre ep_table != NULL, endpoint != NULL
 * @note 调用者需要手动调用shm_comm_recv_zc_end释放消息
 */
shm_base_msg_t* shm_comm_recv_zc(shm_endpoint_table_t* ep_table, shm_endpoint_t* endpoint);

/**
 * @brief 完成零拷贝接收操作
 * 
 * 释放通过shm_comm_recv_zc接收的消息缓冲区。
 * 
 * @param[in] endpoint 源端点
 * @pre endpoint != NULL
 * @post 消息缓冲区被释放，队列位置被更新
 */
void shm_comm_recv_zc_end(shm_endpoint_t* endpoint);

#endif // SHM_COMM_H
