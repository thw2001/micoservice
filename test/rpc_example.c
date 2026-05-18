#include "rpc.h"
#include "cJSON.h"
#include "cf_log.h"
#include <stdio.h>
#include <stdlib.h>

// 示例方法处理器
static cJSON* example_add_handler(rpc_json_context* ctx, cJSON* params, cJSON* id) {
    cJSON* a = cJSON_GetObjectItem(params, "a");
    cJSON* b = cJSON_GetObjectItem(params, "b");
    
    if (!a || !b || a->type != cJSON_Number || b->type != cJSON_Number) {
        ctx->error_code = JRPC_INVALID_PARAMS;
        ctx->error_message = "Invalid parameters: 'a' and 'b' must be numbers";
        return NULL;
    }
    
    double result = a->valuedouble + b->valuedouble;
    cJSON* response = cJSON_CreateNumber(result);
    
    log_info("Add method called: %f + %f = %f", a->valuedouble, b->valuedouble, result);
    return response;
}

// 示例响应回调
static void example_response_callback(void* arg, cJSON* result, int error_code, const char* error_message) {
    if (error_code == 0) {
        if (result && result->type == cJSON_Number) {
            log_info("RPC call successful, result: %f", result->valuedouble);
        } else {
            log_info("RPC call successful, result: %s", cJSON_Print(result));
        }
    } else {
        log_error("RPC call failed: %s (code: %d)", error_message, error_code);
    }
}

// 使用示例
void rpc_usage_example() {
    // 创建abs_comm（这里需要实际的实现）
    // abs_comm* rc = abs_comm_new("client1", &connect_info);
    
    // 创建RPC上下文
    // rpc_context* ctx = rpc_context_new(rc);
    
    // 注册方法
    // rpc_register_method(ctx, "add", example_add_handler, NULL);
    
    // 假设对端client_name为"server1"
    // const char* target_client = "server1";
    // 调用方法 - 方式1：完整的JSON-RPC请求
    // const char* jsonrpc_request1 = "{\"jsonrpc\": \"2.0\", \"method\": \"add\", \"params\": {\"a\": 10, \"b\": 20}, \"id\": \"1\"}";
    // rpc_call_method(ctx, target_client, jsonrpc_request1, example_response_callback, NULL, 5000);
    // 调用方法 - 方式2：不带ID的请求（系统自动生成ID）
    // const char* jsonrpc_request2 = "{\"jsonrpc\": \"2.0\", \"method\": \"add\", \"params\": {\"a\": 30, \"b\": 40}}";
    // rpc_call_method(ctx, target_client, jsonrpc_request2, example_response_callback, NULL, 5000);
    // 调用方法 - 方式3：带数组参数的请求
    // const char* jsonrpc_request3 = "{\"jsonrpc\": \"2.0\", \"method\": \"add\", \"params\": [10, 20], \"id\": \"req3\"}";
    // rpc_call_method(ctx, target_client, jsonrpc_request3, example_response_callback, NULL, 5000);
    // 注意：无需手动添加from字段，rpc_call_method会自动注入，所有响应主题由from字段决定
    // 定时检查超时
    // rpc_check_timeouts(ctx);
    
    // 清理
    // rpc_context_delete(ctx);
    // abs_comm_delete(rc);
    
    log_info("RPC usage example completed");
}

// 服务器端示例
void rpc_server_example() {
    // 创建RPC上下文作为服务器
    // rpc_context* server_ctx = rpc_context_new(server_rc, "server1");
    
    // 注册服务器方法
    // rpc_register_method(server_ctx, "add", example_add_handler, NULL);
    // rpc_register_method(server_ctx, "multiply", multiply_handler, NULL);
    
    // 服务器会通过request_handler自动处理请求
    // 并调用注册的方法处理器
    
    log_info("RPC server example completed");
} 