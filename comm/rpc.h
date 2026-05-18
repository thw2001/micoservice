#ifndef COMM_RPC_H
#define COMM_RPC_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "abs_comm.h"
#include "map.h"
#include "cJSON.h"
#include <pthread.h>
#include <stdint.h>
#include <time.h>

// JSON-RPC 错误码定义
#define JRPC_PARSE_ERROR -32700
#define JRPC_INVALID_REQUEST -32600
#define JRPC_METHOD_NOT_FOUND -32601
#define JRPC_INVALID_PARAMS -32602
#define JRPC_INTERNAL_ERROR -32603

// JSON-RPC 上下文
typedef struct {
    void* data;
    int error_code;
    char* error_message;
} rpc_json_context;

// JSON-RPC 方法处理器类型
typedef cJSON* (*rpc_json_method_handler)(rpc_json_context* ctx, cJSON* params, cJSON* id);

// JSON-RPC 响应回调
typedef void (*rpc_json_response_callback)(void* arg, cJSON* result, int error_code, const char* error_message);

// RPC Method Entry
typedef struct {
    rpc_json_method_handler handler;
    void* data;
} rpc_method_entry;

typedef map_t(rpc_method_entry) map_method_entry_t;

// Pending Request
typedef struct {
    rpc_json_response_callback callback;
    void* arg;
    time_t request_time;
    int timeout_ms;
} rpc_pending_request;

typedef map_t(rpc_pending_request) map_pending_request_t;

// RPC Context
typedef struct rpc_context {
    abs_comm* rc;
    char* client_name; // 本端唯一标识
    char request_topic[32]; // 拼接后的请求主题
    char response_topic[32]; // 拼接后的响应主题
    map_method_entry_t methods;
    map_pending_request_t pending_requests;
    pthread_mutex_t mutex;
} rpc_context;

// 创建RPC上下文，该上下文集客户端和服务段一体，使用rc->client_name作为获取响应的主题生成凭据，server_name作为方法注册的本端唯一标识，大部分情况可一致
rpc_context* rpc_context_new(abs_comm* rc);
void rpc_context_delete(rpc_context* ctx);

// 注册JSON-RPC方法
int rpc_register_method(rpc_context* ctx, const char* method_name, rpc_json_method_handler handler, void* data);

// 调用JSON-RPC方法，target_client_name为目标rpc服务端server_name
int rpc_call_method(rpc_context* ctx, const char* target_client_name, const char* jsonrpc_request,
                    rpc_json_response_callback callback, void* arg, int timeout_ms);

// 同步调用JSON-RPC方法，target_client_name为目标rpc服务端server_name
cJSON* rpc_call_method_sync(rpc_context* ctx, const char* target_client_name, const char* jsonrpc_request, int timeout_ms, int* error_code, char** error_message);

// 超时检查（需定时调用）
void rpc_check_timeouts(rpc_context* ctx);

#ifdef __cplusplus
}
#endif
#endif // COMM_RPC_H
