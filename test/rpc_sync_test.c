#include "rpc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// 模拟的注册中心
typedef struct {
    char* client_name;
    int (*pub_msg)(struct abs_comm* rc, char* topic, char* msg, int len);
} abs_comm;

// 模拟发布消息的函数
int mock_pub_msg(abs_comm* rc, char* topic, char* msg, int len) {
    printf("Publishing message to topic: %s\n", topic);
    printf("Message: %.*s\n", len, msg);
    return 0;
}

// 示例方法处理器
cJSON* example_handler(rpc_json_context* ctx, cJSON* params, cJSON* id) {
    cJSON* result = cJSON_CreateString("Hello from example handler!");
    return result;
}

int main() {
    // 创建模拟的注册中心
    abs_comm rc;
    rc.client_name = "test_client";
    rc.pub_msg = mock_pub_msg;
    
    // 创建RPC上下文
    rpc_context* ctx = rpc_context_new(&rc);
    if (!ctx) {
        printf("Failed to create RPC context\n");
        return -1;
    }
    
    // 注册示例方法
    rpc_register_method(ctx, "example_method", example_handler, NULL);
    
    // 创建JSON-RPC请求
    cJSON* request = cJSON_CreateObject();
    cJSON_AddStringToObject(request, "method", "example_method");
    cJSON_AddStringToObject(request, "from", "test_client");
    cJSON_AddNumberToObject(request, "id", 123);
    
    char* json_request = cJSON_PrintUnformatted(request);
    cJSON_Delete(request);
    
    printf("Sending request: %s\n", json_request);
    
    // 使用同步方法调用（这里我们只是测试函数是否能正常编译和链接）
    int error_code;
    char* error_message;
    cJSON* result = rpc_call_method_sync(ctx, "target_client", json_request, 5000, &error_code, &error_message);
    
    if (result) {
        char* result_str = cJSON_PrintUnformatted(result);
        printf("Received result: %s\n", result_str);
        free(result_str);
        cJSON_Delete(result);
    } else {
        printf("Error occurred, code: %d, message: %s\n", error_code, error_message ? error_message : "Unknown error");
        if (error_message) {
            free(error_message);
        }
    }
    
    free(json_request);
    rpc_context_delete(ctx);
    
    return 0;
}
