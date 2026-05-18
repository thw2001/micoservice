#include "shm_endpoint.h"
#include "shm_errors.h"
#include "shm_log.h"
#include <string.h>
#include <stdio.h>

/**
 * @brief 验证端点名称是否有效
 * 
 * @param name 端点名称
 * @return true 名称有效
 * @return false 名称无效
 */
static bool validate_endpoint_name(const char* name) {
    return (name != NULL) && (strlen(name) > 0) && (strlen(name) < MAX_ENDPOINT_NAME);
}

int shm_endpoint_table_init(shm_endpoint_table_t* table, uint32_t size) {
    if (!table) {
        log_error("Endpoint table init failed: invalid table pointer");
        return SHM_INVALID_PARAM;
    }

    if (size < sizeof(shm_endpoint_table_t))
    {
        log_error("Endpoint table init failed: size too small. size: %u < %lu", size, sizeof(shm_endpoint_table_t));
        return SHM_INVALID_PARAM;
    }
    

    // 初始化锁
    int ret = shm_lock_init(&table->lock);
    if (ret != SHM_SUCCESS) {
        log_error("Endpoint table init failed: lock init failed with error %d", ret);
        return ret;
    }
    
    table->offset_in_shm = sizeof(shm_endpoint_table_t);
    table->count = 0;
    table->size = size;

    log_debug("Endpoint table initialized successfully with size=%u", size);
    return SHM_SUCCESS;
}

int shm_endpoint_register_base(shm_endpoint_table_t* table, const char* name, uint32_t data_size, uint32_t size) {
    if (!table || !name) {
        log_error("Endpoint register failed: invalid parameters");
        return SHM_INVALID_PARAM;
    }
    
    if (!validate_endpoint_name(name)) {
        log_error("Endpoint register failed: invalid endpoint name '%s'", name);
        return SHM_INVALID_PARAM;
    }
    
    if (size == 0 || data_size == 0) {
        log_error("Endpoint register failed: invalid queue size data_size=%u, size=%u", data_size, size);
        return SHM_QUEUE_INVALID_SIZE;
    }

    if (table->offset_in_shm + data_size * size > table->size) {
        log_error("Endpoint register failed: insufficient memory, required=%u, available=%u", 
                 table->offset_in_shm + data_size * size, table->size);
        return SHM_INVALID_MEMORY;
    }
    
    int ret = SHM_SUCCESS;
    // 获取锁
    shm_lock_acquire(&table->lock);

    // 检查端点表是否已满
    if (table->count >= MAX_ENDPOINTS) {
        log_error("Endpoint register failed: endpoint table is full (max=%d)", MAX_ENDPOINTS);
        shm_lock_release(&table->lock);
        return SHM_ENDPOINT_TABLE_FULL;
    }

    // 检查端点名称是否已经存在
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->endpoints[i].name, name) == 0) {
            log_error("Endpoint register failed: endpoint '%s' already exists", name);
            shm_lock_release(&table->lock);
            return SHM_ENDPOINT_EXISTS;
        }
    }

    // 创建新端点
    shm_endpoint_t* endpoint = &table->endpoints[table->count];
    strncpy(endpoint->name, name, MAX_ENDPOINT_NAME - 1);
    endpoint->name[MAX_ENDPOINT_NAME - 1] = '\0';

    // 初始化统计信息
    endpoint->bytes_sent = 0;
    endpoint->bytes_received = 0;
    endpoint->send_errors = 0;
    endpoint->receive_errors = 0;
    endpoint->total_sends = 0;
    endpoint->total_receives = 0;

    ret = shm_queue_init(&endpoint->queue, table->offset_in_shm, data_size, size);
    if (ret != SHM_SUCCESS) {
        log_error("Endpoint register failed: queue init failed with error %d", ret);
        shm_lock_release(&table->lock);
        return ret;
    }

    table->count++;
    table->offset_in_shm += data_size * size;
    
    // 释放锁
    shm_lock_release(&table->lock);
    
    log_info("Endpoint '%s' registered successfully, queue_size=%u, data_size=%u", name, size, data_size);
    return SHM_SUCCESS;
}

int shm_endpoint_unregister(shm_endpoint_table_t* table, const char* name) {
    if (!table || !name) {
        log_error("Endpoint unregister failed: invalid parameters");
        return SHM_INVALID_PARAM;
    }
    
    if (!validate_endpoint_name(name)) {
        log_error("Endpoint unregister failed: invalid endpoint name '%s'", name);
        return SHM_INVALID_PARAM;
    }
    
    shm_lock_acquire(&table->lock);
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->endpoints[i].name, name) == 0) {
            // 清理被删除端点的资源
            shm_endpoint_t* endpoint = &table->endpoints[i];
            
            // 重置端点的统计信息
            endpoint->bytes_sent = 0;
            endpoint->bytes_received = 0;
            endpoint->send_errors = 0;
            endpoint->receive_errors = 0;
            endpoint->total_sends = 0;
            endpoint->total_receives = 0;
            
            // 清空端点名称
            memset(endpoint->name, 0, sizeof(endpoint->name));
            
            // 重置队列信息
            memset(&endpoint->queue, 0, sizeof(endpoint->queue));
            
            // 将最后一个端点移动到当前删除的位置（如果需要）
            if (i < table->count - 1) {
                // 使用memmove而不是直接赋值，确保内存安全
                memmove(&table->endpoints[i], &table->endpoints[table->count - 1], 
                       sizeof(shm_endpoint_t));
                
                // 清理原来最后一个端点的位置
                memset(&table->endpoints[table->count - 1], 0, sizeof(shm_endpoint_t));
            } else {
                // 如果是最后一个端点，直接清零
                memset(&table->endpoints[i], 0, sizeof(shm_endpoint_t));
            }
            
            table->count--;
            shm_lock_release(&table->lock);
            log_info("Endpoint '%s' unregistered successfully", name);
            return SHM_SUCCESS;
        }
    }
    shm_lock_release(&table->lock);

    log_warn("Endpoint unregister failed: endpoint '%s' not found", name);
    return SHM_ENDPOINT_NOT_FOUND;
}

shm_endpoint_t* shm_endpoint_find(shm_endpoint_table_t* table, const char* name) {
    if (!table || !name) {
        log_error("Endpoint find failed: invalid parameters");
        return NULL;
    }
    
    if (!validate_endpoint_name(name)) {
        log_error("Endpoint find failed: invalid endpoint name '%s'", name);
        return NULL;
    }
    
    shm_endpoint_t* found_endpoint = NULL;
    
    shm_lock_acquire(&table->lock);
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->endpoints[i].name, name) == 0) {
            found_endpoint = &table->endpoints[i];
            break;
        }
    }
    shm_lock_release(&table->lock);
    
    return found_endpoint;
}

void shm_endpoint_table_print(shm_endpoint_table_t* table) {
    if (!table) {
        log_error("Endpoint table print failed: invalid table pointer");
        return;
    }

    log_info("\n=== Endpoint Table ===");
    log_info("Total endpoints: %d", table->count);
    log_info("--------------------");

    shm_lock_acquire(&table->lock);
    
    for (int i = 0; i < table->count; i++) {
        shm_endpoint_t* endpoint = &table->endpoints[i];
        log_info("Endpoint[%d]:", i);
        log_info("  Name: %s", endpoint->name);
        log_info("  Queue offset: %u", endpoint->queue.offset);
        log_info("  Queue size: %u", endpoint->queue.size);
        log_info("  Queue data_size: %u", endpoint->queue.data_size);
        log_info("--------------------");
    }

    shm_lock_release(&table->lock);
    log_info("=== End of Table ===\n");
}

void shm_endpoint_table_print_stats(shm_endpoint_table_t* table) {
    if (!table) {
        log_error("Endpoint statistics print failed: invalid table pointer");
        return;
    }

    log_info("\n=== Endpoint Statistics ===");
    log_info("Total endpoints: %d", table->count);
    log_info("------------------------------------------------------------");

    shm_lock_acquire(&table->lock);
    
    for (int i = 0; i < table->count; i++) {
        shm_endpoint_t* endpoint = &table->endpoints[i];
        log_info("Endpoint[%d]: %s", i, endpoint->name);
        log_info("  Bytes Sent: %llu", (unsigned long long)endpoint->bytes_sent);
        log_info("  Bytes Received: %llu", (unsigned long long)endpoint->bytes_received);
        log_info("  Total Sends: %u", endpoint->total_sends);
        log_info("  Total Receives: %u", endpoint->total_receives);
        log_info("  Send Errors: %u", endpoint->send_errors);
        log_info("  Receive Errors: %u", endpoint->receive_errors);
        
        // 计算成功率
        float send_success_rate = 0.0f;
        float receive_success_rate = 0.0f;
        
        if (endpoint->total_sends > 0) {
            send_success_rate = 100.0f * (1.0f - (float)endpoint->send_errors / endpoint->total_sends);
        }
        
        if (endpoint->total_receives > 0) {
            receive_success_rate = 100.0f * (1.0f - (float)endpoint->receive_errors / endpoint->total_receives);
        }
        
        log_info("  Send Success Rate: %.2f%%", send_success_rate);
        log_info("  Receive Success Rate: %.2f%%", receive_success_rate);
        log_info("------------------------------------------------------------");
    }

    shm_lock_release(&table->lock);
    log_info("=== End of Statistics ===\n");
}
