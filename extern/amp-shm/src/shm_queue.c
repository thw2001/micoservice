#include "shm_queue.h"
#include "shm_errors.h"
#include "shm_log.h"
#include <string.h>
#include <errno.h>
#include <stdbool.h>

/**
 * @brief 验证队列参数是否有效
 * @param queue 队列指针
 * @return 有效返回true，无效返回false
 */
bool shm_queue_validate_params(const shm_queue_t* queue) {
    return (queue != NULL) && (queue->size > 0) && (queue->data_size > 0);
}

/**
 * @brief 计算消息在共享内存中的位置
 * @param base_addr 共享内存基地址
 * @param queue 队列指针
 * @param index 消息索引
 * @return 消息指针，无效参数返回NULL
 */
void* shm_queue_calc_msg_position(void* base_addr, const shm_queue_t* queue, uint32_t index) {
    if (base_addr == NULL || !shm_queue_validate_params(queue)) {
        log_error("Invalid parameters in shm_queue_calc_msg_position");
        return NULL;
    }
    
    if (index >= queue->size) {
        log_error("Index %u out of bounds (size: %u)", index, queue->size);
        return NULL;
    }
    
    return (char*)base_addr + queue->offset + index * queue->data_size;
}

int shm_queue_init(shm_queue_t* queue, uint32_t offset, uint32_t data_size, uint32_t size) {
    // 前置条件检查
    if (queue == NULL) {
        log_error("Queue pointer is NULL");
        return SHM_INVALID_PARAM;
    }
    
    if (data_size == 0 || size == 0) {
        log_error("Invalid parameters: data_size=%u, size=%u", 
                 data_size, size);
        return SHM_INVALID_PARAM;
    }

    // 边界检查：队列大小必须大于1（因为环形队列需要一个空位区分满和空）
    if (size < 2) {
        log_error("Queue size too small: %u (minimum 2 required)", size);
        return SHM_QUEUE_INVALID_SIZE;
    }

    // 边界检查：数据大小不能超过合理限制
    if (data_size > 1024 * 1024) { // 1MB限制
        log_error("Data size too large: %u (maximum 1MB allowed)", data_size);
        return SHM_QUEUE_DATA_TOO_LARGE;
    }

    // 边界检查：偏移量不能过大
    if (offset > UINT32_MAX - data_size * size) {
        log_error("Invalid offset: %u, would cause integer overflow", offset);
        return SHM_INVALID_PARAM;
    }

    // 初始化队列结构
    memset(queue, 0, sizeof(shm_queue_t)); // 清零所有字段
    queue->head = 0;
    queue->tail = 0;
    queue->size = size;
    queue->offset = offset;
    queue->data_size = data_size;
    
    log_debug("Queue initialized: size=%u, data_size=%u, offset=%u", 
             size, data_size, offset);
    return SHM_SUCCESS;
}

int shm_queue_is_empty(shm_queue_t* queue) {
    if (!shm_queue_validate_params(queue)) {
        log_error("Queue parameters are invalid");
        return SHM_INVALID_PARAM;
    }
    
    return (queue->head == queue->tail) ? 1 : 0;
}

int shm_queue_is_full(shm_queue_t* queue) {
    if (!shm_queue_validate_params(queue)) {
        log_error("Queue parameters are invalid");
        return SHM_INVALID_PARAM;
    }
    
    return ((queue->head + 1) % queue->size == queue->tail) ? 1 : 0;
}

int shm_queue_push(void* base_addr, shm_queue_t* queue, void* data, uint32_t size) {
    if (base_addr == NULL || queue == NULL || data == NULL) {
        log_error("Invalid parameters: base_addr=%p, queue=%p, data=%p", 
                 base_addr, queue, data);
        return SHM_INVALID_PARAM;
    }
    
    if (size == 0) {
        log_error("Data size is zero");
        return SHM_INVALID_PARAM;
    }
    
    if (!shm_queue_validate_params(queue)) {
        log_error("Queue parameters are invalid");
        return SHM_INVALID_PARAM;
    }
    
    if (size > queue->data_size) {
        log_error("Data size %u exceeds queue data size %u", size, queue->data_size);
        return SHM_QUEUE_DATA_TOO_LARGE;
    }
    
    int full = shm_queue_is_full(queue);
    if (full < 0) {
        return full; // 返回错误码
    }
    if (full) {
        log_warn("Queue is full, cannot push data");
        return SHM_QUEUE_FULL;
    }

    // 计算写入位置
    void* write_ptr = shm_queue_calc_msg_position(base_addr, queue, queue->head);
    if (write_ptr == NULL) {
        log_error("Failed to calculate write position");
        return SHM_INTERNAL_ERROR;
    }

    shm_base_msg_t* msg = (shm_base_msg_t*)write_ptr;
    msg->type = MSG_TYPE_BASE;
    msg->size = size;

    // 写入数据
    memcpy(msg->payload, data, size);
    
    // 更新head指针
    queue->head = (queue->head + 1) % queue->size;
    
    log_trace("Data pushed to queue: size=%u, head=%lu", size, queue->head);
    return SHM_SUCCESS;
}

shm_base_msg_t* shm_queue_push_alloc(void* base_addr, shm_queue_t* queue, uint32_t size) {
    if (base_addr == NULL || queue == NULL) {
        log_error("Invalid parameters: base_addr=%p, queue=%p", base_addr, queue);
        return NULL;
    }
    
    if (size == 0) {
        log_error("Data size is zero");
        return NULL;
    }
    
    if (!shm_queue_validate_params(queue)) {
        log_error("Queue parameters are invalid");
        return NULL;
    }
    
    if (size > queue->data_size) {
        log_error("Data size %u exceeds queue data size %u", size, queue->data_size);
        return NULL;
    }
    
    int full = shm_queue_is_full(queue);
    if (full < 0) {
        return NULL; // 返回错误
    }
    if (full) {
        log_warn("Queue is full, cannot allocate space");
        return NULL;
    }

    // 计算写入位置
    void* write_ptr = shm_queue_calc_msg_position(base_addr, queue, queue->head);
    if (write_ptr == NULL) {
        log_error("Failed to calculate write position");
        return NULL;
    }

    shm_base_msg_t* msg = (shm_base_msg_t*)write_ptr;
    msg->size = size;
    
    log_trace("Space allocated in queue: size=%u, head=%lu", size, queue->head);
    return msg;
}

void shm_queue_push_alloc_end(void* base_addr, shm_queue_t* queue) {
    if (base_addr == NULL || !shm_queue_validate_params(queue)) {
        log_error("Invalid parameters: base_addr=%p, queue=%p", base_addr, queue);
        return;
    }
    
    // 更新head指针
    queue->head = (queue->head + 1) % queue->size;
    log_trace("Zero-copy push completed, head=%lu", queue->head);
}

int shm_queue_pop(void* base_addr, shm_queue_t* queue, void* data, uint32_t size) {
    if (base_addr == NULL || queue == NULL || data == NULL) {
        log_error("Invalid parameters: base_addr=%p, queue=%p, data=%p", 
                 base_addr, queue, data);
        return SHM_INVALID_PARAM;
    }
    
    if (size == 0) {
        log_error("Buffer size is zero");
        return SHM_INVALID_PARAM;
    }
    
    if (!shm_queue_validate_params(queue)) {
        log_error("Queue parameters are invalid");
        return SHM_INVALID_PARAM;
    }
    
    int empty = shm_queue_is_empty(queue);
    if (empty < 0) {
        return empty; // 返回错误码
    }
    if (empty) {
        log_trace("Queue is empty, cannot pop data");
        return SHM_QUEUE_EMPTY;
    }

    // 计算读取位置
    void* read_ptr = shm_queue_calc_msg_position(base_addr, queue, queue->tail);
    if (read_ptr == NULL) {
        log_error("Failed to calculate read position");
        return SHM_INTERNAL_ERROR;
    }
    
    shm_base_msg_t* msg = (shm_base_msg_t*)read_ptr;

    if (msg->size > size) {
        log_error("Message size %u exceeds buffer size %u", msg->size, size);
        return SHM_QUEUE_DATA_FLOW;
    }
    
    // 读取数据
    memcpy(data, msg->payload, msg->size);
    
    // 更新tail指针
    queue->tail = (queue->tail + 1) % queue->size;
    
    log_trace("Data popped from queue: size=%u, tail=%lu", msg->size, queue->tail);
    return msg->size;
}

shm_base_msg_t* shm_queue_pop_zc(void* base_addr, shm_queue_t* queue) {
    if (base_addr == NULL || queue == NULL) {
        log_error("Invalid parameters: base_addr=%p, queue=%p", base_addr, queue);
        return NULL;
    }
    
    if (!shm_queue_validate_params(queue)) {
        log_error("Queue parameters are invalid");
        return NULL;
    }
    
    int empty = shm_queue_is_empty(queue);
    if (empty < 0) {
        return NULL; // 返回错误
    }
    if (empty) {
        log_trace("Queue is empty, cannot pop data");
        return NULL;
    }

    // 计算读取位置
    void* read_ptr = shm_queue_calc_msg_position(base_addr, queue, queue->tail);
    if (read_ptr == NULL) {
        log_error("Failed to calculate read position");
        return NULL;
    }
    
    shm_base_msg_t* msg = (shm_base_msg_t*)read_ptr;
    log_trace("Zero-copy pop from queue: size=%u, tail=%lu", msg->size, queue->tail);
    return msg;
}

void shm_queue_pop_zc_end(shm_queue_t* queue) {
    if (!shm_queue_validate_params(queue)) {
        log_error("Queue parameters are invalid");
        return;
    }
    
    // 更新tail指针
    queue->tail = (queue->tail + 1) % queue->size;
    log_trace("Zero-copy pop completed, tail=%lu", queue->tail);
}
