/** 
 * @file shm_queue.h
 * @brief 共享内存队列管理模块
 * @copyright 2023 AMP-SHM Project, MIT License
 * 
 * 提供基于共享内存的循环队列实现，支持零拷贝消息传递。
 * 线程安全：单读单写场景下无锁，多线程操作需要外部同步。
 */

#ifndef SHM_QUEUE_H
#define SHM_QUEUE_H

#include <stdint.h>
#include "shm_errors.h"

/** @brief 消息类型定义 */
enum {
    MSG_TYPE_BASE = 0,      /**< 基础消息类型 */
    MSG_TYPE_PUB_SUB,       /**< 发布订阅消息类型 */
};

/** 
 * @brief 基础消息结构体
 * @note 使用柔性数组存储实际消息数据
 */
typedef struct {
    uint32_t size;          /**< 消息数据大小 */
    uint8_t type;           /**< 消息类型 */
    uint8_t payload[];      /**< 消息数据柔性数组 */
} shm_base_msg_t;

#pragma pack(1)

/**
 * @brief 共享内存队列结构体
 * @note 使用1字节对齐确保跨进程兼容性
 */
typedef struct {
    uintptr_t head;         /**< 队列头指针 */
    uintptr_t tail;         /**< 队列尾指针 */
    uint32_t size;          /**< 循环队列的条数 */
    uint32_t data_size;     /**< 每条消息实际能使用的大小 */
    uint32_t offset;        /**< 队列相对于共享内存起始地址的偏移量 */
} shm_queue_t;
#pragma pack()

/**
 * @brief 初始化共享内存队列
 * @param[out] queue 队列结构体指针
 * @param[in] offset 队列在共享内存中的偏移量
 * @param[in] data_size 单条消息数据大小
 * @param[in] size 队列容量（重要：因为内部为环形队列，所以消息容量是队列大小-1）
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 * @pre queue != NULL, offset > 0, data_size > 0, size > 0
 * @post 队列头尾指针初始化为0，其他字段设置为指定值
 */
int shm_queue_init(shm_queue_t* queue, uint32_t offset, uint32_t data_size, uint32_t size);

/**
 * @brief 检查队列是否为空
 * @param[in] queue 队列结构体指针
 * @return 1表示空，0表示非空，错误返回负数错误码
 * @pre queue != NULL
 */
int shm_queue_is_empty(shm_queue_t* queue);

/**
 * @brief 检查队列是否已满
 * @param[in] queue 队列结构体指针
 * @return 1表示满，0表示未满，错误返回负数错误码
 * @pre queue != NULL
 */
int shm_queue_is_full(shm_queue_t* queue);

/**
 * @brief 消息入队操作（带数据拷贝）
 * @param[in] base_addr 共享内存基地址
 * @param[in] queue 队列结构体指针
 * @param[in] data 要入队的数据指针
 * @param[in] size 数据大小
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 * @pre base_addr != NULL, queue != NULL, data != NULL, size > 0
 * @post 数据被拷贝到队列中，head指针更新
 */
int shm_queue_push(void* base_addr, shm_queue_t* queue, void* data, uint32_t size);

/**
 * @brief 消息入队操作（零拷贝分配）
 * @param[in] base_addr 共享内存基地址
 * @param[in] queue 队列结构体指针
 * @param[in] size 要分配的消息大小
 * @return 成功返回消息指针，失败返回NULL
 * @pre base_addr != NULL, queue != NULL, size > 0
 * @note 调用者需要在消息指针上填充数据后调用shm_queue_push_alloc_end
 */
shm_base_msg_t* shm_queue_push_alloc(void* base_addr, shm_queue_t* queue, uint32_t size);

/**
 * @brief 零拷贝入队完成操作
 * @param[in] base_addr 共享内存基地址
 * @param[in] queue 队列结构体指针
 * @pre base_addr != NULL, queue != NULL
 * @post head指针更新，消息正式入队
 */
void shm_queue_push_alloc_end(void* base_addr, shm_queue_t* queue);

/**
 * @brief 消息出队操作（带数据拷贝）
 * @param[in] base_addr 共享内存基地址
 * @param[in] queue 队列结构体指针
 * @param[out] data 接收数据的缓冲区指针
 * @param[in] size 缓冲区大小
 * @return 成功返回实际读取的数据大小，失败返回错误码
 * @pre base_addr != NULL, queue != NULL, data != NULL, size > 0
 * @post 数据从队列拷贝到缓冲区，tail指针更新
 */
int shm_queue_pop(void* base_addr, shm_queue_t* queue, void* data, uint32_t size);

/**
 * @brief 消息出队操作（零拷贝）
 * @param[in] base_addr 共享内存基地址
 * @param[in] queue 队列结构体指针
 * @return 成功返回消息指针，失败返回NULL
 * @pre base_addr != NULL, queue != NULL
 * @note 调用者处理完消息后需要调用shm_queue_pop_zc_end
 */
shm_base_msg_t* shm_queue_pop_zc(void* base_addr, shm_queue_t* queue);

/**
 * @brief 零拷贝出队完成操作
 * @param[in] queue 队列结构体指针
 * @pre queue != NULL
 * @post tail指针更新，消息正式出队
 */
void shm_queue_pop_zc_end(shm_queue_t* queue);

#endif // SHM_QUEUE_H
