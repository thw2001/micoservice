/**
 * @file shm_endpoint.h
 * @brief 共享内存端点管理模块
 * @copyright 2023 AMP-SHM Project, MIT License
 * 
 * 提供共享内存通信端点的注册、查找和管理功能。支持多端点并发访问，
 * 使用进程间互斥锁保证线程安全。
 * 
 * @thread_safety 线程安全，使用进程间互斥锁保护共享数据结构
 * @memory 端点表在共享内存中分配，需要外部管理生命周期
 */

#ifndef SHM_ENDPOINT_H
#define SHM_ENDPOINT_H

#include <stdint.h>
#include <stdbool.h>
#include "shm_queue.h"
#include "shm_lock.h"
#include "shm_errors.h"

/** @brief 端点名称最大长度 */
#define MAX_ENDPOINT_NAME 128

/** @brief 最大端点数量 */
#define MAX_ENDPOINTS 32

#pragma pack(1)

/**
 * @brief 端点统计信息结构体
 * 
 * 用于监控端点的通信状态和健康状况
 */
typedef struct {
    uint64_t bytes_sent;          /**< 发送字节总数 */
    uint64_t bytes_received;      /**< 接收字节总数 */
    uint32_t send_errors;         /**< 发送错误次数 */
    uint32_t receive_errors;      /**< 接收错误次数 */
    uint32_t total_sends;         /**< 总发送次数 */
    uint32_t total_receives;      /**< 总接收次数 */
    
    char name[MAX_ENDPOINT_NAME]; /**< 端点名称 */
    shm_queue_t queue;            /**< 关联的消息队列 */
} shm_endpoint_t;

/**
 * @brief 端点表结构体
 * 
 * 管理所有注册的端点，提供线程安全的访问接口
 */
typedef struct {
    int count;                    /**< 当前端点数量 */
    uint32_t size;                /**< 共享内存总大小 */
    shm_lock_t lock;              /**< 进程间互斥锁 */
    uint32_t offset_in_shm;       /**< 队列缓冲区在共享内存中的偏移量 */
    shm_endpoint_t endpoints[MAX_ENDPOINTS]; /**< 端点数组 */
} shm_endpoint_table_t;

#pragma pack()

/** @brief 计算端点缓冲区起始地址的宏 */
#define shm_endpoint_buffer(ptr) ((char*)ptr + sizeof(shm_endpoint_table_t))

/**
 * @brief 初始化端点表
 * 
 * @param[in,out] table 端点表指针
 * @param[in] size 共享内存总大小
 * @return int 成功返回SHM_SUCCESS，失败返回错误码
 * @pre table != NULL, size > 0
 * @post 端点表被初始化为空状态，锁被初始化，size被设置
 */
int shm_endpoint_table_init(shm_endpoint_table_t* table, uint32_t size);

/**
 * @brief 注册新端点
 * 
 * @param[in,out] table 端点表指针
 * @param[in] name 端点名称
 * @param[in] data_size 消息数据大小
 * @param[in] size 队列容量
 * @return int 成功返回SHM_SUCCESS，失败返回错误码
 * @pre table != NULL, name != NULL, data_size > 0, size > 0
 * @pre strlen(name) < MAX_ENDPOINT_NAME
 * @post 新端点被添加到端点表中，分配相应的队列空间
 */
int shm_endpoint_register_base(shm_endpoint_table_t* table, const char* name, uint32_t data_size, uint32_t size);

/**
 * @brief 注销端点
 * 
 * @param[in,out] table 端点表指针
 * @param[in] name 端点名称
 * @return int 成功返回SHM_SUCCESS，失败返回错误码
 * @pre table != NULL, name != NULL
 * @post 指定端点从端点表中移除
 */
int shm_endpoint_unregister(shm_endpoint_table_t* table, const char* name);

/**
 * @brief 查找端点
 * 
 * @param[in] table 端点表指针
 * @param[in] name 端点名称
 * @return shm_endpoint_t* 成功返回端点指针，失败返回NULL
 * @pre table != NULL, name != NULL
 * @thread_safety 线程安全
 */
shm_endpoint_t* shm_endpoint_find(shm_endpoint_table_t* table, const char* name);

/**
 * @brief 打印端点表信息
 * 
 * @param[in] table 端点表指针
 * @pre table != NULL
 * @thread_safety 线程安全
 */
void shm_endpoint_table_print(shm_endpoint_table_t* table);

/**
 * @brief 打印端点统计信息
 * 
 * @param[in] table 端点表指针
 * @pre table != NULL
 * @thread_safety 线程安全
 */
void shm_endpoint_table_print_stats(shm_endpoint_table_t* table);

#endif // SHM_ENDPOINT_H
