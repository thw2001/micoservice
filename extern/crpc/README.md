# CRPC

## 1. 介绍

CRPC是一个c语言实现的简单的RPC通信框架。服务端采用单进程单线程、事件循环结构处理客户端请求。本框架适用于io密集型，而计算密集型的RPC请求则会阻塞后续请求。本框架还支持异步通知，也即服务端接收客户端注册事件，并在事件发生时可通知给客户端。

宏 | 
WITH_PUB_SUB | 使用发布订阅模式时开启这个宏
---

## 2. 代码结构

    CRPC
    ├── README.md
    ├── include 头文件定义
    │   ├── client.h 客户端头文件
    │   ├── common.h 通用定义
    │   ├── context.h 通信上下文定义
    │   ├── evloop.h 事件循环
    │   ├── hash_table.h hash表定义
    │   ├── log.h 日志输出定义
    │   ├── net.h 网络函数封装
    │   ├── serial.h 序列化定义
    │   └── server.h 服务端头文件
    ├── src 实现
    │   ├── client.c
    │   ├── context.c
    │   ├── evloop.c
    │   ├── hash_table.c
    │   ├── makefile
    │   ├── net.c
    │   ├── serial.c
    │   └── server.c
    └── test 测试
        ├── json_rpc_client.c
        └── json_rpc_server.c
        └── pubsub_test_broker.c
        └── pubsub_test_client.c
        └── rpc_client.c
        └── rpc_server.c

---

**注意事项**：
1. 单次传输最大数据量大小为`CONTEXT_BUFFER_MAX_SIZE`，接收数据单包大小为`MAX_ONE_LINE_SIZE`，若发送大数据，需要调整这些，发送数据时会自动的把大包拆成多个MAX_ONE_LINE_SIZE的大小的包，包数拆的多会非常影响性能。
2. 传字符串时的分隔符是`'\t'`，消息结束标志为`"$$$$$$"`。在传字符串时避免出现这个，可以采用UStr传输，不需要考虑这些。
3. 目前数据传输的主要性能瓶颈在：
   - 使用strstr寻找消息结束标志
   - 接收消息的逻辑是这样的：每次socket有数据到来，会进入消息处理循环，等到socket fd的读事件，即每次接收到包时会去调用`ContextGetReadRecord`函数去分辨当前缓冲区的包是不是一个完整的包，即strstr下一个消息结束标志存不存在，这个过程中，如果不存在，就会calloc free strstr一次，大包拆分的性能瓶颈在这里。所以只能用于io密集型，数据传输效率很低

## 3. 使用说明

### 3.1 服务端

- 服务端实现时首先需要创建服务`CreateRpcServer`
- 再执行`RunRpcLoop`进入循环，等待客户端消息
- 服务端需设置`ServerOnTransact`回调，再该函数中进行反序列化客户端请求，并根据不同的请求调用不同处理结果
- 服务端也需要设置`ServerOnCallbackTransact`与`ServerEndCallbackTransact`回调函数，这两个函数用于事件通知


例子见test/rpc_server.c


### 3.2 客户端

- 客户端首先创建实例`CreateRpcClient`
- 再请求时使用序列化函数`WriteBegin`标识开始写入，`WriteEnd`标识写入完成
- 客户端请求时可能为多线程方式，但RPC需要顺序处理，所以再每次请求RPC时需要首先执行`LockRpcClient`，并再请求结束后执行`UnlockRpcClient`
- 客户端需要设置`OnTransact`回调处理事件通知

例子见test/rpc_client.c