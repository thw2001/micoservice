#include "rpc.h"
#include "map.h"
#include "pthread.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "cf_log.h"

// 订阅和发布主题拼接
#define TOPIC_REQUEST_FMT "rpc/req/%s"
#define TOPIC_RESPONSE_FMT "rpc/resp/%s"

/**
 * @brief 生成唯一的correlation_id字符串（时间戳+自增计数）
 * @return 动态分配的字符串，需free
 */
static char* generate_correlation_id() {
    static long counter = 0;
    char* id = malloc(64);
    snprintf(id, 64, "%ld-%ld", time(NULL), __sync_fetch_and_add(&counter, 1));
    return id;
}

/**
 * @brief 发送JSON-RPC错误响应到指定主题
 * @param ctx rpc上下文
 * @param topic 目标主题
 * @param code 错误码
 * @param message 错误消息
 * @param id 请求id
 * @return 0/错误码
 */
static int send_jsonrpc_error(rpc_context* ctx, const char* topic, int code, const char* message, cJSON* id) {
    cJSON* error_root = cJSON_CreateObject();
    cJSON* result_root = cJSON_CreateObject();
    
    cJSON_AddNumberToObject(error_root, "code", code);
    cJSON_AddStringToObject(error_root, "message", message);
    cJSON_AddItemToObject(result_root, "error", error_root);
    if (id) {
        cJSON_AddItemToObject(result_root, "id", id);
    }
    
    char* str_result = cJSON_PrintUnformatted(result_root);
    int ret = ctx->rc->pub_msg(ctx->rc, (char*)topic, str_result, strlen(str_result));
    
    free(str_result);
    cJSON_Delete(result_root);
    return ret;
}

/**
 * @brief 发送JSON-RPC成功响应到指定主题
 * @param ctx rpc上下文
 * @param topic 目标主题
 * @param result 结果对象
 * @param id 请求id
 * @return 0/错误码
 */
static int send_jsonrpc_result(rpc_context* ctx, const char* topic, cJSON* result, cJSON* id) {
    cJSON* result_root = cJSON_CreateObject();
    if (result) {
        cJSON_AddItemToObject(result_root, "result", result);
    }
    if (id) {
        cJSON_AddItemToObject(result_root, "id", id);
    }
    
    char* str_result = cJSON_PrintUnformatted(result_root);
    log_debug("topic: %s, result: %s", topic, str_result);
    int ret = ctx->rc->pub_msg(ctx->rc, (char*)topic, str_result, strlen(str_result));
    
    free(str_result);
    cJSON_Delete(result_root);
    return ret;
}

/**
 * @brief 调用已注册的JSON-RPC方法，并将结果/错误响应发回from指定的主题
 * @param ctx rpc上下文
 * @param method_name 方法名
 * @param params 参数对象
 * @param id 请求id
 * @param from_client 请求方client_name
 * @return 0/错误码
 */
static int invoke_jsonrpc_method(rpc_context* ctx, const char* method_name, cJSON* params, cJSON* id, const char* from_client) {
    cJSON* returned = NULL;
    int procedure_found = 0;
    rpc_json_context rpc_ctx;
    rpc_ctx.error_code = 0;
    rpc_ctx.error_message = NULL;
    
    // 查找注册的方法
    rpc_method_entry* entry = map_get(&ctx->methods, method_name);
    if (entry) {
        procedure_found = 1;
        rpc_ctx.data = entry->data;
        returned = entry->handler(&rpc_ctx, params, id);
    }
    
    // 获取from字段决定响应主题
    if (!from_client) {
        log_error("[rpc] missing 'from' field in request, cannot send response");
        return -1;
    }
    char topic_resp[32];
    snprintf(topic_resp, sizeof(topic_resp), TOPIC_RESPONSE_FMT, from_client);
    
    if (!procedure_found) {
        return send_jsonrpc_error(ctx, topic_resp, JRPC_METHOD_NOT_FOUND, 
                                 "Method not found.", id);
    } else {
        if (rpc_ctx.error_code) {
            return send_jsonrpc_error(ctx, topic_resp, rpc_ctx.error_code, 
                                     rpc_ctx.error_message, id);
        } else {
            return send_jsonrpc_result(ctx, topic_resp, returned, id);
        }
    }
}

/**
 * @brief 解析JSON-RPC请求，提取method/params/id，调用invoke_jsonrpc_method
 * @param ctx rpc上下文
 * @param root 解析后的cJSON对象
 * @param from_client 请求方client_name
 * @return 0/错误码
 */
static int eval_jsonrpc_request(rpc_context* ctx, cJSON* root, const char* from_client) {
    cJSON *method, *params, *id;
    
    method = cJSON_GetObjectItem(root, "method");
    if (method != NULL && method->type == cJSON_String) {
        params = cJSON_GetObjectItem(root, "params");
        if (params == NULL || params->type == cJSON_Array || params->type == cJSON_Object) {
            id = cJSON_GetObjectItem(root, "id");
            if (id == NULL || id->type == cJSON_String || id->type == cJSON_Number) {
                // 复制ID用于响应
                cJSON* id_copy = NULL;
                if (id != NULL) {
                    id_copy = (id->type == cJSON_String) ? 
                              cJSON_CreateString(id->valuestring) :
                              cJSON_CreateNumber(id->valueint);
                }
                
                log_debug("JSON-RPC Method Invoked: %s", method->valuestring);
                return invoke_jsonrpc_method(ctx, method->valuestring, params, id_copy, from_client);
            }
        }
    }
    
    if (from_client) {
        char topic_resp[32];
        snprintf(topic_resp, sizeof(topic_resp), TOPIC_RESPONSE_FMT, from_client);
        return send_jsonrpc_error(ctx, topic_resp, JRPC_INVALID_REQUEST,
                             "The JSON sent is not a valid Request object.", NULL);
    }
    return -1;
}

/**
 * @brief 处理JSON-RPC请求消息的入口，负责解析from字段、错误处理、分发到方法调用
 * @param arg rpc_context*
 * @param msg 收到的消息内容
 * @param len 消息长度
 * @return 0
 */
static int request_handler(void* arg, void* msg, uint32_t len) {
    rpc_context* ctx = (rpc_context*)arg;
    char* msg_str = (char*)msg;
    
    // 确保消息以null结尾
    if (len > 0 && msg_str[len-1] != '\0') {
        char* temp = malloc(len + 1);
        memcpy(temp, msg_str, len);
        temp[len] = '\0';
        msg_str = temp;
    }

    log_debug("%s", msg_str);
    
    cJSON* root = cJSON_Parse(msg_str);
    // 先解析from字段
    const char* from_client = NULL;
    if (root && root->type == cJSON_Object) {
        cJSON* from = cJSON_GetObjectItem(root, "from");
        if (from && cJSON_IsString(from)) {
            from_client = from->valuestring;
        }
    }
    if (!root || root->type != cJSON_Object) {
        log_error("Invalid JSON-RPC request format.");
        if (from_client) {
            char topic_resp[32];
            snprintf(topic_resp, sizeof(topic_resp), TOPIC_RESPONSE_FMT, from_client);
            send_jsonrpc_error(ctx, topic_resp, JRPC_INVALID_REQUEST, "Invalid JSON-RPC request format.", NULL);
        }
        if (root) cJSON_Delete(root);
        if (msg_str != (char*)msg) free(msg_str);
        return 0;
    }
    // 传from_client给后续调用
    eval_jsonrpc_request(ctx, root, from_client);
    cJSON_Delete(root);
    if (msg_str != (char*)msg) free(msg_str);
    return 0;
}

/**
 * @brief 处理JSON-RPC响应消息，匹配pending_requests并回调
 * @param arg rpc_context*
 * @param msg 收到的消息内容
 * @param len 消息长度
 * @return 0
 */
static int response_handler(void* arg, void* msg, uint32_t len) {
    rpc_context* ctx = (rpc_context*)arg;
    char* msg_str = (char*)msg;
    
    // 确保消息以null结尾
    if (len > 0 && msg_str[len-1] != '\0') {
        char* temp = malloc(len + 1);
        memcpy(temp, msg_str, len);
        temp[len] = '\0';
        msg_str = temp;
    }

    log_debug("%s", msg_str);
    
    cJSON* root = cJSON_Parse(msg_str);
    if (root) {
        cJSON* id = cJSON_GetObjectItem(root, "id");
        if (id) {
            char* correlation_id = NULL;
            if (id->type == cJSON_String) {
                correlation_id = strdup(id->valuestring);
            } else if (id->type == cJSON_Number) {
                correlation_id = malloc(32);
                snprintf(correlation_id, 32, "%d", (int)id->valueint);
            }
            
            if (correlation_id) {
                pthread_mutex_lock(&ctx->mutex);
                rpc_pending_request* pending = map_get(&ctx->pending_requests, correlation_id);
                if (pending && pending->callback) {
                    cJSON* result = cJSON_GetObjectItem(root, "result");
                    cJSON* error = cJSON_GetObjectItem(root, "error");
                    
                    if (error) {
                        cJSON* error_code = cJSON_GetObjectItem(error, "code");
                        cJSON* error_message = cJSON_GetObjectItem(error, "message");
                        int code = error_code ? (int)error_code->valueint : JRPC_INTERNAL_ERROR;
                        const char* message = error_message ? error_message->valuestring : "Unknown error";
                        pending->callback(pending->arg, NULL, code, message);
                    } else {
                        pending->callback(pending->arg, result, 0, NULL);
                    }
                    
                    map_remove(&ctx->pending_requests, correlation_id);
                }
                pthread_mutex_unlock(&ctx->mutex);
                free(correlation_id);
            }
        }
        cJSON_Delete(root);
    }
    
    if (msg_str != (char*)msg) {
        free(msg_str);
    }
    
    return 0;
}

/**
 * @brief 创建rpc上下文，初始化方法表、pending表、主题等
 * @param rc 注册中心指针
 * @return rpc_context*
 */
rpc_context* rpc_context_new(abs_comm* rc) {
    rpc_context* ctx = (rpc_context*)malloc(sizeof(rpc_context));
    ctx->rc = rc;
    ctx->client_name = strdup(rc->client_name);
    map_init(&ctx->methods);
    map_init(&ctx->pending_requests);
    pthread_mutex_init(&ctx->mutex, NULL);
    // 主题拼接并存储，长度不超过31
    snprintf(ctx->request_topic, sizeof(ctx->request_topic), TOPIC_REQUEST_FMT, ctx->client_name);
    snprintf(ctx->response_topic, sizeof(ctx->response_topic), TOPIC_RESPONSE_FMT, ctx->client_name);
    // 订阅本端请求和响应主题
    log_debug("ctx->request_topic: %s, ctx->response_topic: %s", ctx->request_topic, ctx->response_topic);
    abs_comm_sub(rc, ctx->request_topic, ctx, request_handler);
    abs_comm_sub(rc, ctx->response_topic, ctx, response_handler);
    return ctx;
}

/**
 * @brief 销毁rpc上下文，释放资源
 * @param ctx rpc_context*
 */
void rpc_context_delete(rpc_context* ctx) {
    if (!ctx) return;
    map_deinit(&ctx->methods);
    map_deinit(&ctx->pending_requests);
    pthread_mutex_destroy(&ctx->mutex);
    if (ctx->client_name) free(ctx->client_name);
    free(ctx);
}

/**
 * @brief 注册JSON-RPC方法
 * @param ctx rpc_context*
 * @param method_name 方法名
 * @param handler 方法处理器
 * @param data 用户数据
 * @return 0/错误码
 */
int rpc_register_method(rpc_context* ctx, const char* method_name, rpc_json_method_handler handler, void* data) {
    if (!ctx) return -1;
    rpc_method_entry entry = {handler, data};
    map_set(&ctx->methods, method_name, entry);
    return 0;
}

/**
 * @brief 以JSON-RPC协议调用远端方法，自动注入from字段，支持回调和超时
 * @param ctx rpc_context*
 * @param target_client_name 目标rpc服务端client_name
 * @param jsonrpc_request JSON-RPC请求字符串
 * @param callback 响应回调
 * @param arg 回调参数
 * @param timeout_ms 超时时间ms
 * @return 0/错误码
 */
int rpc_call_method(rpc_context* ctx, const char* target_client_name, const char* jsonrpc_request,
                    rpc_json_response_callback callback, void* arg, int timeout_ms) {
    if (!ctx) return -1;
    cJSON* request = cJSON_Parse(jsonrpc_request);
    if (!request) {
        log_error("Invalid JSON-RPC request format");
        return -1;
    }
    // 提取ID，如果不存在则生成一个
    cJSON* id = cJSON_GetObjectItem(request, "id");
    char* correlation_id = NULL;
    if (id) {
        if (id->type == cJSON_String) {
            correlation_id = strdup(id->valuestring);
        } else if (id->type == cJSON_Number) {
            correlation_id = malloc(32);
            snprintf(correlation_id, 32, "%d", (int)id->valueint);
        }
    }
    if (!correlation_id) {
        correlation_id = generate_correlation_id();
        cJSON_DeleteItemFromObject(request, "id");
        cJSON_AddStringToObject(request, "id", correlation_id);
    }
    // 注入from字段到顶层
    cJSON* from_item = cJSON_GetObjectItem(request, "from");
    if (from_item) {
        cJSON_ReplaceItemInObject(request, "from", cJSON_CreateString(ctx->client_name));
    } else {
        cJSON_AddStringToObject(request, "from", ctx->client_name);
    }
    rpc_pending_request pending = {
        .callback = callback,
        .arg = arg,
        .request_time = time(NULL),
        .timeout_ms = timeout_ms
    };
    pthread_mutex_lock(&ctx->mutex);
    map_set(&ctx->pending_requests, correlation_id, pending);
    pthread_mutex_unlock(&ctx->mutex);
    char topic_req[32];
    snprintf(topic_req, sizeof(topic_req), TOPIC_REQUEST_FMT, target_client_name);
    char* str_request = cJSON_PrintUnformatted(request);
    int ret = ctx->rc->pub_msg(ctx->rc, topic_req, str_request, strlen(str_request));
    free(str_request);
    cJSON_Delete(request);
    free(correlation_id);
    return ret;
}

/**
 * @brief 检查所有pending请求是否超时，超时则回调并清理
 * @param ctx rpc_context*
 */
void rpc_check_timeouts(rpc_context* ctx) {
    if (!ctx) return;
    pthread_mutex_lock(&ctx->mutex);
    map_iter_t iter = map_iter(&ctx->pending_requests);
    const char* key;
    time_t now = time(NULL);
    while ((key = map_next(&ctx->pending_requests, &iter))) {
        rpc_pending_request* pending = map_get(&ctx->pending_requests, key);
        if (pending && pending->timeout_ms > 0 && now - pending->request_time > pending->timeout_ms / 1000) {
            log_debug("rpc_call timeout: %s", key);
            if (pending->callback) {
                pending->callback(pending->arg, NULL, JRPC_INTERNAL_ERROR, "Request timeout");
            }
            map_remove(&ctx->pending_requests, key);
        }
    }
    pthread_mutex_unlock(&ctx->mutex);
}

// 同步调用的上下文
typedef struct {
    pthread_cond_t cond;        // 条件变量
    pthread_mutex_t mutex;      // 互斥锁
    cJSON* result;              // 返回结果
    int error_code;             // 错误码
    char* error_message;        // 错误消息
    int completed;              // 完成标志
} rpc_sync_context;

/**
 * @brief 同步调用的回调函数
 * @param arg 同步上下文
 * @param result 返回结果
 * @param error_code 错误码
 * @param error_message 错误消息
 */
static void rpc_sync_callback(void* arg, cJSON* result, int error_code, const char* error_message) {
    rpc_sync_context* sync_ctx = (rpc_sync_context*)arg;
    
    pthread_mutex_lock(&sync_ctx->mutex);
    
    // 设置结果
    if (result) {
        sync_ctx->result = cJSON_Duplicate(result, 1);  // 复制结果
    }
    sync_ctx->error_code = error_code;
    if (error_message) {
        sync_ctx->error_message = strdup(error_message);
    }
    sync_ctx->completed = 1;
    
    // 通知等待的线程
    pthread_cond_signal(&sync_ctx->cond);
    pthread_mutex_unlock(&sync_ctx->mutex);
}

/**
 * @brief 同步调用JSON-RPC方法，target_client_name为目标rpc服务端client_name
 * @param ctx rpc_context*
 * @param target_client_name 目标rpc服务端client_name
 * @param jsonrpc_request JSON-RPC请求字符串
 * @param timeout_ms 超时时间ms
 * @param error_code 返回的错误码
 * @param error_message 返回的错误消息，如果不为NULL，需要手动free
 * @return cJSON* 返回结果，需调用者释放
 */
cJSON* rpc_call_method_sync(rpc_context* ctx, const char* target_client_name, const char* jsonrpc_request, int timeout_ms, int* error_code, char** error_message) {
    if (!ctx) {
        if (error_code) *error_code = -1;
        if (error_message) *error_message = strdup("Invalid context");
        return NULL;
    }
    
    // 初始化同步上下文
    rpc_sync_context sync_ctx;
    pthread_cond_init(&sync_ctx.cond, NULL);
    pthread_mutex_init(&sync_ctx.mutex, NULL);
    sync_ctx.result = NULL;
    sync_ctx.error_code = 0;
    sync_ctx.error_message = NULL;
    sync_ctx.completed = 0;
    
    // 调用异步方法
    int ret = rpc_call_method(ctx, target_client_name, jsonrpc_request, rpc_sync_callback, &sync_ctx, timeout_ms);
    if (ret != 0) {
        pthread_cond_destroy(&sync_ctx.cond);
        pthread_mutex_destroy(&sync_ctx.mutex);
        if (error_code) *error_code = ret;
        if (error_message) *error_message = strdup("Failed to call method");
        return NULL;
    }
    
    // 等待结果或超时
    pthread_mutex_lock(&sync_ctx.mutex);
    if (!sync_ctx.completed) {
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += timeout_ms / 1000;
        timeout.tv_nsec += (timeout_ms % 1000) * 1000000;
        
        // 处理纳秒溢出
        if (timeout.tv_nsec >= 1000000000) {
            timeout.tv_sec++;
            timeout.tv_nsec -= 1000000000;
        }
        
        int wait_result = pthread_cond_timedwait(&sync_ctx.cond, &sync_ctx.mutex, &timeout);
        if (wait_result == ETIMEDOUT) {
            sync_ctx.error_code = JRPC_INTERNAL_ERROR;
            sync_ctx.error_message = strdup("Request timeout");
        }
    }
    pthread_mutex_unlock(&sync_ctx.mutex);
    
    // 清理资源
    pthread_cond_destroy(&sync_ctx.cond);
    pthread_mutex_destroy(&sync_ctx.mutex);
    
    // 返回结果
    if (error_code) *error_code = sync_ctx.error_code;
    if (error_message) *error_message = sync_ctx.error_message;
    return sync_ctx.result;
}
