# 微服务框架

微服务是一系列独立部署、运行的小型服务，这些小型服务组合成分布式的系统。微服务框架是用来管理微服务的，包含多种组件。

微服务框架中主要分为两个核心组件，注册中心和微服务

## to-do list

- [ ] node_center和registry中获取的分布式日志的处理与存储, node_center_recv_msg:82, registry_sub_topic_callback:91，研究日志的传输逻辑，设计一个日志的滚动存储方式；
- [ ] deploy_cmd_rpc_handler中node_loader_exec执行错误会导致这个函数执行两次，疑似fork的
- [ ] 微服务生命周期，安装、启动、停止、卸载

- **注册中心**

注册中心接收所有微服务的注册信息

## 服务治理

微服务的服务治理是指在微服务架构中，为了确保各个独立部署、运行的小型服务能够高效协作，提供稳定可靠的服务而采取的一系列管理和技术措施。

### 服务发现

### 负载均衡

### 容错处理


## 日志链路追踪

使用opentracing的思想，采用Trace tree和Span作为链路追踪。

在分布式链路追踪中，主要的流程包括以下几个步骤：

1. 生成Trace：当一个请求首次进入系统时，会创建一个新的追踪（Trace）。这个追踪是一个全局唯一的标识符，用来贯穿整个请求处理流程。
2. 创建Span：每个服务或组件在处理请求的过程中，都会为自己的操作创建一个或多个（Span）。Span是追踪的基本单位，表示一个逻辑单元的工作，比如一次数据库查询或调用另一个微服务。每个Span包含开始时间和结束时间，以及描述该操作的元数据，如操作名称、标签等。
3. 传播Context：为了确保所有相关的Spans能够关联起来形成一个完整的Trace，需要在服务间传递追踪上下文（SpanContext）。
4. 记录事件和注释：除了基本的操作信息外，还可以在Spans中记录额外的信息，如事件发生的时间点（例如，当一个请求被发送或接收到时），或者是关于操作执行情况的注释。
5. 收集和报告：一旦操作完成，Spans会被发送到一个集中系统(维护Trace tree)进行分析。这些系统可以提供可视化工具来帮助用户查看请求的执行路径，分析性能问题或者故障原因。
6. 分析和诊断：最后，通过集中式的追踪数据，可以对系统的整体健康状况进行监控，识别并解决潜在的问题。


Span:
```json
{
  "traceId": "1234567890abcdef1234567890abcdef",
  "spanId": "1234567890abcdef",
  "parentSpanId": "0f1e2d3c4b5a6978",  // 可选，表示父 Span 的 ID
  "operationName": "getUserData",
  "startTime": "2024-11-13T17:48:00Z",
  "endTime": "2024-11-13T17:48:01Z",
  "duration": 1000,  // 单位为毫秒
  "tags": {
    "http.method": "GET",
    "http.url": "/api/user/12345",
    "component": "web-server"
  },
  "logs": [
    {
      "timestamp": "2024-11-13T17:48:00.123Z",
      "fields": {
        "event": "start",
        "message": "Request received"
      }
    },
    {
      "timestamp": "2024-11-13T17:48:00.567Z",
      "fields": {
        "event": "database.query",
        "message": "Executing database query"
      }
    }
  ],
  "references": [
    {
      "refType": "CHILD_OF",
      "traceId": "1234567890abcdef1234567890abcdef",
      "spanId": "0f1e2d3c4b5a6978"
    }
  ],
  "warnings": [],
  "errors": []
}
```

span的可视化：

OpenTracing 中的 Trace（调用链）通过归属于此调用链的 Span 来隐性的定义。
特别说明，一条 Trace（调用链）可以被认为是一个由多个 Span 组成的有向无环图（DAG图），Span 与 Span 的关系被命名References。

例如：下面的示例 Trace 就是由8个 Span 组成：

```
单个 Trace 中，span 间的因果关系


        [Span A]  ←←←(the root span)
            |
     +------+------+
     |             |
 [Span B]      [Span C] ←←←(Span C 是 Span A 的孩子节点, ChildOf)
     |             |
 [Span D]      +---+-------+
               |           |
           [Span E]    [Span F] >>> [Span G] >>> [Span H]
                                       ↑
                                       ↑
                                       ↑
                         (Span G 在 Span F 后被调用, FollowsFrom)

```

基于时间轴的时序图可以更好的展现 Trace（调用链）：

```
单个 Trace 中，span 间的时间关系


––|–––––––|–––––––|–––––––|–––––––|–––––––|–––––––|–––––––|–> time

 [Span A···················································]
   [Span B··············································]
      [Span D··········································]
    [Span C········································]
         [Span E·······]        [Span F··] [Span G··] [Span H··]

```


SpanContext:
```json
{
  "traceId": "1234567890abcdef1234567890abcdef",
  "spanId": "1234567890abcdef",
  "parentSpanId": "0f1e2d3c4b5a6978",  // 可选，表示父 Span 的 ID
  "sampled": true,
  "baggage": { // 可选，这个会携带整个链路
    "user_id": "12345",
    "session_id": "abcde12345"
  }
}
```

链路追踪可单独作为一个项目，需实现两个模块：
- Trace日志集中分析管理模块
- span模块

## 微服务发布与管理

包管理中心
命令执行器

## 通信中间件

crpc