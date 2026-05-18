/**
 * @file shm_errors.h
 * @brief 共享内存通信错误码定义
 * @copyright 2023 AMP-SHM Project, MIT License
 * 
 * 定义共享内存通信模块使用的错误码，按照功能模块进行分类。
 * 所有错误码均为负值，成功返回SHM_SUCCESS (0)。
 */

#ifndef SHM_ERRORS_H
#define SHM_ERRORS_H

/** @brief 成功 */
#define SHM_SUCCESS               0   /**< 操作成功 */

// 通用错误码 (1-49)
#define SHM_INVALID_PARAM        -1   /**< 无效参数 */
#define SHM_MEMORY_ERROR         -2   /**< 内存错误 */
#define SHM_INTERNAL_ERROR       -3   /**< 内部错误 */
#define SHM_INVALID_MEMORY       -4   /**< 无效内存区域 */

// 锁相关错误码 (50-99)
#define SHM_LOCK_INIT_FAILED     -50  /**< 锁初始化失败 */
#define SHM_LOCK_OPERATION_FAILED -51 /**< 锁操作失败 */

// 队列模块错误码 (100-199)
#define SHM_QUEUE_INVALID_SIZE   -101 /**< 无效队列大小 */
#define SHM_QUEUE_FULL           -102 /**< 队列已满 */
#define SHM_QUEUE_EMPTY          -103 /**< 队列为空 */
#define SHM_QUEUE_DATA_TOO_LARGE -104 /**< 数据超过块大小 */
#define SHM_QUEUE_DATA_FLOW      -105 /**< 传入的暂存区大小小于数据大小 */

// 端点模块错误码 (200-299)
#define SHM_ENDPOINT_NOT_FOUND   -201 /**< 端点未找到 */
#define SHM_ENDPOINT_EXISTS      -202 /**< 端点已存在 */
#define SHM_ENDPOINT_TABLE_FULL  -203 /**< 端点表已满 */

// 通信模块错误码 (300-399)
#define SHM_COMM_BUFFER_TOO_SMALL -301 /**< 缓冲区太小 */
#define SHM_COMM_TIMEOUT         -302 /**< 操作超时 */
#define SHM_COMM_DISCONNECTED    -303 /**< 连接断开 */
#define SHM_COMM_CONFIG_EXISTS   -304 /**< 配置已存在 */
#define SHM_COMM_ID_EXISTS       -305 /**< ID已存在 */
#define SHM_COMM_NAME_EXISTS     -306 /**< 名称已存在 */

// 通信模块错误码 (400-499)
#define PUB_MSG_HAVE_FAILED      -401 /**< 缓冲区太小 */
#define SHM_INVALID_CONFIG       -402 /**< 无效配置 */
#define PUB_HAVE_NO_TOPIC        -403 /**< 不存在对应主题 */

// JSON配置处理模块错误码 (500-599)
#define SHM_FILE_ERROR           -501 /**< 文件操作错误 */
#define SHM_NOT_IMPLEMENTED      -502 /**< 功能未实现 */
#define JSON_CONFIG_INVALID_JSON     -503 /**< 无效的JSON格式 */
#define JSON_CONFIG_MERGE_CONFLICT   -504 /**< 配置合并冲突 */
#define JSON_CONFIG_VALIDATION_FAIL  -505 /**< 配置验证失败 */
#define JSON_CONFIG_MEMORY_ERROR     -506 /**< 内存分配失败 */


#endif // SHM_ERRORS_H
