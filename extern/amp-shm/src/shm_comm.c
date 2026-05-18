/**
 * @file shm_comm.c
 * @brief 共享内存通信模块实现
 * @copyright 2023 AMP-SHM Project, MIT License
 * 
 * 实现基于共享内存的高性能进程间通信功能，包括共享内存初始化、
 * 通道管理、消息发送接收等核心功能。
 * 
 * @note 线程安全：核心消息传递采用无锁设计，配置管理需要外部同步
 */

#include "shm_comm.h"
#include "shm_errors.h"
#include "shm_log.h"
#include <string.h>

// 全局变量已被移除，所有操作通过配置参数进行

/**
 * @brief 获取端点表指针
 * 
 * @param[in] config 共享内存通信配置
 * @return shm_endpoint_table_t* 端点表指针，如果配置无效返回NULL
 */
shm_endpoint_table_t* shm_get_endpoint_table(shm_comm_config_t* config) {
    if (!config) {
        log_error("shm_get_endpoint_table: invalid config");
        return NULL;
    }
    return config->ep_table;
}

/**
 * @brief 打印共享内存通信统计信息
 * 
 * 输出所有端点的发送接收统计信息。
 * 
 * @param[in] config 共享内存通信配置
 */
void shm_print_stats(shm_comm_config_t* config){
    if (!config || !config->ep_table) {
        log_error("shm_print_stats: invalid config or endpoint table");
        return;
    }
    shm_endpoint_table_print_stats(config->ep_table);
}

/**
 * @brief 从进程共享内存初始化
 * 
 * 从进程调用此函数连接到已存在的共享内存区域。
 * 
 * @param[in,out] config 共享内存通信配置
 * @param[in] shm_addr 共享内存起始地址
 * @return int 成功返回SHM_SUCCESS，失败返回错误码
 */
int shm_comm_init_minor(shm_comm_config_t* config, void* shm_addr) {
    if (!config || !shm_addr) {
        log_error("shm_comm_init_minor: invalid parameters");
        return SHM_INVALID_PARAM;
    }

    config->ep_table = (shm_endpoint_table_t*)shm_addr;

    log_debug("minor process initialized with shared memory at %p", shm_addr);
    return SHM_SUCCESS;
}

int shm_comm_deinit(shm_comm_config_t* config){
    if (!config) {
        log_error("shm_comm_deinit: invalid config parameter");
        return SHM_INVALID_PARAM;
    }

    // 记录当前的通道数量用于日志
    int channel_count = config->channel_num;
    
    // 清理通道配置
    config->channel_num = 0;
    memset(config->channels, 0, sizeof(config->channels));
    
    // 注意：ep_table指向共享内存，由外部管理，这里不需要释放
    // 只需要将指针置为NULL，避免悬挂指针
    config->ep_table = NULL;
    
    log_debug("shm_comm_deinit: cleaned up config with %d channels", channel_count);
    return SHM_SUCCESS; 
}

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
 */
int shm_comm_config_add_channel(shm_comm_config_t* config, const char* sender_name, const char* recver_name, 
                              uint32_t data_size, uint32_t size) {
    if (!config || !sender_name || !recver_name) {
        log_error("shm_comm_config_add_channel: invalid parameters");
        return SHM_INVALID_PARAM;
    }
    if (config->channel_num >= MAX_NODE_NUM) {
        log_error("shm_comm_config_add_channel: maximum channel number (%d) exceeded", 
                 MAX_NODE_NUM);
        return SHM_INVALID_PARAM;
    }
    if (strlen(sender_name) >= MAX_ENDPOINT_NAME || 
        strlen(recver_name) >= MAX_ENDPOINT_NAME) {
        log_error("shm_comm_config_add_channel: name too long (max %d characters)", 
                 MAX_ENDPOINT_NAME - 1);
        return SHM_INVALID_PARAM;
    }

    // 安全地复制字符串
    strncpy(config->channels[config->channel_num].sender_name, 
           sender_name, MAX_ENDPOINT_NAME - 1);
    config->channels[config->channel_num].sender_name[MAX_ENDPOINT_NAME - 1] = '\0';
    strncpy(config->channels[config->channel_num].recver_name,
           recver_name, MAX_ENDPOINT_NAME - 1);
    config->channels[config->channel_num].recver_name[MAX_ENDPOINT_NAME - 1] = '\0';
    
    config->channels[config->channel_num].data_size = data_size;
    config->channels[config->channel_num].size = size;
    config->channel_num++;

    log_info("added channel: %s -> %s (data_size: %u, queue_size: %u)", 
             sender_name, recver_name, data_size, size);
    log_debug("total channels: %d", config->channel_num);

    return SHM_SUCCESS;
}

/**
 * @brief 计算所需共享内存大小
 * 
 * 根据当前配置计算所需的共享内存总大小。
 * 
 * @param[in] config 共享内存通信配置
 * @return int 所需内存大小（字节）
 */
int shm_comm_cal_mem(shm_comm_config_t* config) {
    if (!config) {
        log_error("shm_comm_cal_mem: invalid config");
        return SHM_INVALID_PARAM;
    }
    
    int mem = sizeof(shm_endpoint_table_t);
    for (int i = 0; i < config->channel_num; i++) {
        mem += config->channels[i].data_size * config->channels[i].size;
    }
    log_debug("calculated memory requirement: %d bytes", mem);
    return mem;
}

/**
 * @brief 生成端点名称
 * 
 * 根据发送者和接收者名称生成唯一的端点名称。
 * 
 * @param[out] buffer 输出缓冲区
 * @param[in] sender_name 发送者名称
 * @param[in] receiver_name 接收者名称
 */
static void generate_endpoint_name(char* buffer, const char* sender_name, const char* receiver_name) {
    snprintf(buffer, MAX_ENDPOINT_NAME, "%s->%s", sender_name, receiver_name);
}

/**
 * @brief 加载配置到共享内存
 * 
 * 将配置中的通道信息加载到共享内存，创建对应的端点。
 * 
 * @param[in,out] config 共享内存通信配置
 * @return int 成功返回SHM_SUCCESS，失败返回错误码
 */
int shm_comm_load_config(shm_comm_config_t* config) {
    if (!config || !config->ep_table) {
        log_error("shm_comm_load_config: invalid config or endpoint table");
        return SHM_INVALID_PARAM;
    }

    char endpoint_name[MAX_ENDPOINT_NAME];
    int ret;

    log_debug("loading %d channel configurations", config->channel_num);
    
    for (int i = 0; i < config->channel_num; i++) {
        generate_endpoint_name(endpoint_name,
            config->channels[i].sender_name,
            config->channels[i].recver_name);
        
        log_debug("creating endpoint: %s (data_size: %u, queue_size: %u)",
                 endpoint_name, config->channels[i].data_size, 
                 config->channels[i].size);
        
        ret = shm_comm_create_endpoint_base(config, endpoint_name,
            config->channels[i].data_size,
            config->channels[i].size);
        
        if (ret < 0 && ret != SHM_ENDPOINT_EXISTS) {
            log_error("failed to create endpoint %s: error %d", endpoint_name, ret);
            return ret;
        }
    }
    
    shm_endpoint_table_print(config->ep_table);
    log_info("configuration loaded successfully");
    return SHM_SUCCESS;
}

/**
 * @brief 清除共享内存中的数据
 * 
 * 重置端点表，清除所有端点的统计信息和队列数据。
 * 
 * @param[in,out] config 共享内存通信配置
 * @return int 成功返回SHM_SUCCESS，失败返回错误码
 */
int shm_comm_clear(shm_comm_config_t* config) {
    if (!config || !config->ep_table) {
        log_error("shm_comm_clear: invalid config or endpoint table");
        return SHM_MEMORY_ERROR;
    }
    
    log_debug("clearing endpoint table with %d endpoints", config->ep_table->count);
    config->ep_table->count = 0;
    memset(config->ep_table->endpoints, 0, sizeof(config->ep_table->endpoints));
    
    log_debug("endpoint table cleared successfully");
    return SHM_SUCCESS;
}

/**
 * @brief 创建新的通信端点
 * 
 * 在共享内存中创建新的通信端点。
 * 
 * @param[in,out] config 共享内存通信配置
 * @param[in] name 端点名称
 * @param[in] data_size 单条消息数据大小（字节）
 * @param[in] size 队列容量（因为内部为环形队列，所以消息容量是队列大小-1）
 * @return int 成功返回SHM_SUCCESS，失败返回错误码
 */
int shm_comm_create_endpoint_base(shm_comm_config_t* config, const char* name, uint32_t data_size, uint32_t size) {
    if (!config || !config->ep_table || !name) {
        log_error("shm_comm_create_endpoint_base: invalid parameters");
        return SHM_INVALID_PARAM;
    }
    
    log_debug("creating endpoint: %s (data_size: %u, queue_size: %u)", name, data_size, size);
    return shm_endpoint_register_base(config->ep_table, name, data_size, size);
}

/**
 * @brief 获取指定通道的端点
 * 
 * 根据发送者和接收者名称查找对应的通信端点。
 * 
 * @param[in] config 共享内存通信配置
 * @param[in] sender 发送者名称
 * @param[in] recver 接收者名称
 * @return shm_endpoint_t* 找到返回端点指针，未找到返回NULL
 */
shm_endpoint_t* shm_comm_get_channel(shm_comm_config_t* config, const char* sender, const char* recver) {
    if (!config || !config->ep_table || !sender || !recver) {
        log_error("shm_comm_get_channel: invalid parameters");
        return NULL;
    }
    
    char endpoint_name[MAX_ENDPOINT_NAME];
    generate_endpoint_name(endpoint_name, sender, recver);
    
    log_debug("looking up channel: %s", endpoint_name);
    return shm_endpoint_find(config->ep_table, endpoint_name);
}

int shm_comm_init_main(shm_comm_config_t* config, void* shm_addr, uint32_t size){
    // 前置条件检查
    if (!shm_addr || !config) {
        log_error("shm_comm_init_main: invalid parameters");
        return SHM_INVALID_PARAM;
    }

    // 边界检查：共享内存大小必须足够容纳端点表
    if (size < sizeof(shm_endpoint_table_t)) {
        log_error("shm_comm_init_main: shared memory size %u too small, minimum %lu required", 
                 size, sizeof(shm_endpoint_table_t));
        return SHM_INVALID_MEMORY;
    }

    // 边界检查：配置对象必须有效
    if (config->channel_num < 0 || config->channel_num > MAX_NODE_NUM) {
        log_error("shm_comm_init_main: invalid channel_num %d", config->channel_num);
        return SHM_INVALID_CONFIG;
    }

    // 计算所需的最小内存大小
    int required_size = shm_comm_cal_mem(config);
    if (required_size < 0) {
        log_error("shm_comm_init_main: failed to calculate required memory size");
        return required_size;
    }

    if ((uint32_t)required_size > size) {
        log_error("shm_comm_init_main: insufficient memory. required: %d, available: %u", 
                 required_size, size);
        return SHM_INVALID_MEMORY;
    }

    // 共享内存布局：
    // 前sizeof(shm_endpoint_table_t)字节用于端点表
    // 剩余空间用于各个端点的环形队列缓冲区
    config->ep_table = (shm_endpoint_table_t*)shm_addr;

    log_debug("main process initialized with shared memory at %p, size: %u B, required: %d B", 
             shm_addr, size, required_size);

    return shm_endpoint_table_init(config->ep_table, size);
}


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
 */
int shm_comm_send(shm_endpoint_table_t* ep_table, shm_endpoint_t* endpoint, void* data, uint32_t size) {
    if (!ep_table || !endpoint || !data) {
        log_error("shm_comm_send: invalid parameters");
        return SHM_INVALID_PARAM;
    }
    
    int result = shm_queue_push(ep_table, &endpoint->queue, data, size);
    
    // 更新统计信息
    if (result == SHM_SUCCESS) {
        endpoint->bytes_sent += size;
        endpoint->total_sends++;
    } else {
        endpoint->send_errors++;
    }
    
    return result;
}

/**
 * @brief 分配发送消息缓冲区（零拷贝）
 * 
 * 为发送操作分配消息缓冲区，支持零拷贝发送。
 * 
 * @param[in] ep_table 端点表指针
 * @param[in] endpoint 目标端点
 * @param[in] size 需要分配的大小（字节）
 * @return shm_base_msg_t* 成功返回消息缓冲区指针，失败返回NULL
 */
shm_base_msg_t* shm_comm_send_alloc(shm_endpoint_table_t* ep_table, shm_endpoint_t* endpoint, uint32_t size) {
    if (!ep_table || !endpoint) {
        log_error("shm_comm_send_alloc: invalid parameters");
        return NULL;
    }

    shm_base_msg_t* sbm = shm_queue_push_alloc(ep_table, &endpoint->queue, size);
    if (NULL == sbm) {
        endpoint->send_errors++;
        log_error("failed to allocate send buffer for endpoint %s, size: %u", 
                 endpoint->name, size);
    } else {
        endpoint->bytes_sent += size;
        endpoint->total_sends++;
    }
    return sbm;
}

/**
 * @brief 完成零拷贝发送操作
 * 
 * 提交通过shm_comm_send_alloc分配的消息缓冲区。
 * 
 * @param[in] ep_table 端点表指针
 * @param[in] endpoint 目标端点
 */
void shm_comm_send_alloc_end(shm_endpoint_table_t* ep_table, shm_endpoint_t* endpoint) {
    if (!ep_table || !endpoint) {
        log_error("shm_comm_send_alloc_end: invalid parameters");
        return;
    }
    
    shm_queue_push_alloc_end(ep_table, &endpoint->queue);
}

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
 */
int shm_comm_recv(shm_endpoint_table_t* ep_table, shm_endpoint_t* endpoint, void* buffer, uint32_t size) {
    if (!ep_table || !endpoint || !buffer) {
        log_error("shm_comm_recv: invalid parameters");
        return SHM_INVALID_PARAM;
    }
    if (size == 0) {
        log_error("shm_comm_recv: buffer size is zero");
        return SHM_COMM_BUFFER_TOO_SMALL;
    }
    
    int result = shm_queue_pop(ep_table, &endpoint->queue, buffer, size);
    if (result > 0) {
        endpoint->bytes_received += result;
        endpoint->total_receives++;
    }
    
    return result;
}

/**
 * @brief 零拷贝接收消息
 * 
 * 以零拷贝方式接收消息，返回消息缓冲区指针。
 * 
 * @param[in] ep_table 端点表指针
 * @param[in] endpoint 源端点
 * @return shm_base_msg_t* 成功返回消息指针，失败返回NULL
 */
shm_base_msg_t* shm_comm_recv_zc(shm_endpoint_table_t* ep_table, shm_endpoint_t* endpoint) {
    if (!ep_table || !endpoint) {
        log_error("shm_comm_recv_zc: invalid parameters");
        return NULL;
    }
    
    shm_base_msg_t* sbm = shm_queue_pop_zc(ep_table, &endpoint->queue);
    if (NULL == sbm) {
    } else {
        endpoint->bytes_received += sbm->size;
        endpoint->total_receives++;
    }

    return sbm;
}

/**
 * @brief 完成零拷贝接收操作
 * 
 * 释放通过shm_comm_recv_zc接收的消息缓冲区。
 * 
 * @param[in] endpoint 源端点
 */
void shm_comm_recv_zc_end(shm_endpoint_t* endpoint) {
    if (!endpoint) {
        log_error("shm_comm_recv_zc_end: invalid endpoint");
        return;
    }
    
    shm_queue_pop_zc_end(&endpoint->queue);
}
