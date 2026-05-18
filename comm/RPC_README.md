# JSON-RPC 模块使用说明

## 概述

本模块实现了基于 JSON-RPC 2.0 规范的 RPC 通信框架，支持方法注册、调用和异步响应处理。

- 每个rpc_context通过client_name唯一标识本端。
- 所有消息主题为`rpc/req/<client_name>`和`rpc/resp/<client_name>`，可多端点共存。
- rpc_call_method需指定目标端client_name。
- 所有请求自动在顶层添加`from`字段，响应主题由该字段决定。

## 主要特性

- 完全兼容 JSON-RPC 2.0 规范
- 支持异步方法调用和响应回调
- 内置超时处理机制
- 线程安全的设计
- 支持错误码和错误消息

## API 接口

### 1. 创建和销毁 RPC 上下文

```c
// 创建 RPC 上下文，client_name为本端唯一标识
rpc_context* rpc_context_new(abs_comm* rc);

// 销毁 RPC 上下文
void rpc_context_delete(rpc_context* ctx);
```

### 2. 注册 JSON-RPC 方法

```c
// 方法处理器类型
typedef cJSON* (*rpc_json_method_handler)(rpc_json_context* ctx, cJSON* params, cJSON* id);

// 注册方法
int rpc_register_method(rpc_context* ctx, const char* method_name, 
                       rpc_json_method_handler handler, void* data);
```

### 3. 调用 JSON-RPC 方法

```c
// 响应回调类型
typedef void (*rpc_json_response_callback)(void* arg, cJSON* result, 
                                         int error_code, const char* error_message);

// 调用方法，target_client_name为目标rpc服务端client_name
// from字段会自动注入到请求顶层
int rpc_call_method(rpc_context* ctx, const char* target_client_name, const char* jsonrpc_request,
                   rpc_json_response_callback callback, void* arg, int timeout_ms);
```

### 4. 超时检查

```c
// 检查超时的请求（需要定时调用）
void rpc_check_timeouts(rpc_context* ctx);
```

## 使用示例

### 服务器端（注册方法）

```c
#include "rpc.h"
#include "cJSON.h"

// 定义方法处理器
static cJSON* add_handler(rpc_json_context* ctx, cJSON* params, cJSON* id) {
    cJSON* a = cJSON_GetObjectItem(params, "a");
    cJSON* b = cJSON_GetObjectItem(params, "b");
    
    if (!a || !b || a->type != cJSON_Number || b->type != cJSON_Number) {
        ctx->error_code = JRPC_INVALID_PARAMS;
        ctx->error_message = "Invalid parameters: 'a' and 'b' must be numbers";
        return NULL;
    }
    
    double result = a->valuedouble + b->valuedouble;
    return cJSON_CreateNumber(result);
}

// 注册方法
rpc_context* ctx = rpc_context_new(rc);
rpc_register_method(ctx, "add", add_handler, NULL);
```

### 客户端（调用方法）

```c
#include "rpc.h"
#include "cJSON.h"

// 定义响应回调
static void response_callback(void* arg, cJSON* result, int error_code, const char* error_message) {
    if (error_code == 0) {
        printf("Result: %f\n", result->valuedouble);
    } else {
        printf("Error: %s (code: %d)\n", error_message, error_code);
    }
}

// 假设目标rpc服务端client_name为"server1"
const char* target_client = "server1";

// 调用方法 - 方式1：完整的JSON-RPC请求
const char* jsonrpc_request1 = "{\"jsonrpc\": \"2.0\", \"method\": \"add\", \"params\": {\"a\": 10, \"b\": 20}, \"id\": \"1\"}";
rpc_call_method(ctx, target_client, jsonrpc_request1, response_callback, NULL, 5000);

// 调用方法 - 方式2：不带ID的请求（系统自动生成ID）
const char* jsonrpc_request2 = "{\"jsonrpc\": \"2.0\", \"method\": \"add\", \"params\": {\"a\": 30, \"b\": 40}}";
rpc_call_method(ctx, target_client, jsonrpc_request2, response_callback, NULL, 5000);

// 调用方法 - 方式3：带数组参数的请求
const char* jsonrpc_request3 = "{\"jsonrpc\": \"2.0\", \"method\": \"add\", \"params\": [10, 20], \"id\": \"req3\"}";
rpc_call_method(ctx, target_client, jsonrpc_request3, response_callback, NULL, 5000);
```

## 主题说明

- 请求主题：`rpc/req/<client_name>`
- 响应主题：`rpc/resp/<from>`（from为请求顶层字段，由rpc_call_method自动注入）
- 每个rpc_context只处理与自身client_name相关的请求和响应
- 这样一个进程既可以作为server也可以作为client，互不影响
- 未带from字段的请求会被拒绝，无法收到响应

## 请求格式说明

### JSON-RPC 请求字符串格式

`rpc_call_method` 函数接收一个符合 JSON-RPC 2.0 规范的请求字符串，格式如下：

```json
{
    "jsonrpc": "2.0",
    "method": "方法名",
    "params": 参数对象或数组,
    "id": "请求ID（可选）"
    // "from": "client_name"  // 由rpc_call_method自动注入
}
```

### 支持的请求格式

1. **完整格式**（推荐）：
```json
{
    "jsonrpc": "2.0",
    "method": "add",
    "params": {"a": 10, "b": 20},
    "id": "123"
}
```

2. **不带ID的格式**（系统自动生成ID）：
```json
{
    "jsonrpc": "2.0",
    "method": "add",
    "params": {"a": 10, "b": 20}
}
```

3. **数组参数格式**：
```json
{
    "jsonrpc": "2.0",
    "method": "add",
    "params": [10, 20],
    "id": "req1"
}
```

4. **无参数格式**：
```json
{
    "jsonrpc": "2.0",
    "method": "getStatus",
    "id": "status1"
}
```

### 成功响应格式
```json
{
    "jsonrpc": "2.0",
    "result": 30,
    "id": "123"
}
```

### 错误响应格式
```json
{
    "jsonrpc": "2.0",
    "error": {
        "code": -32602,
        "message": "Invalid parameters"
    },
    "id": "123"
}
```

## 错误处理

### 请求格式错误
- 如果传入的 `jsonrpc_request` 不是有效的 JSON 格式，函数返回 -1
- 如果 JSON 格式正确但不是有效的 JSON-RPC 2.0 请求，会发送相应的错误响应（响应主题由from字段决定）
- 未带from字段的请求会被拒绝，无法收到响应

### 自动ID生成
- 如果请求中没有 `id` 字段，系统会自动生成一个唯一的ID
- 生成的ID格式为：`时间戳-计数器`

### 超时处理
- 如果请求在指定时间内没有收到响应，会调用回调函数并传递超时错误
- 超时错误码为 `JRPC_INTERNAL_ERROR (-32603)`
- 错误消息为 "Request timeout"

## 注意事项

1. 所有 JSON 对象都需要手动释放内存
2. 超时检查需要定期调用 `rpc_check_timeouts()`
3. 方法处理器中可以通过 `ctx->error_code` 和 `ctx->error_message` 设置错误
4. 响应回调中的 `result` 参数在成功时包含结果，失败时为 NULL
5. `jsonrpc_request` 必须是有效的 JSON-RPC 2.0 格式字符串
6. 如果请求中没有 `id` 字段，系统会自动生成一个 