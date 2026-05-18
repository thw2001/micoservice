# AMP-SHM 共享内存发布订阅中间件

## 项目概述

AMP-SHM是一个高性能、跨进程的共享内存发布订阅中间件，提供基于主题的消息分发机制，支持多对多的通信模式。该项目采用C99标准开发，遵循严格的工程化开发规范，提供完整的单元测试和示例程序。

### 核心特性

- **高性能通信**: 基于System V共享内存的高性能进程间通信
- **发布订阅模式**: 支持多发布者、多订阅者的消息分发
- **零拷贝传输**: 支持零拷贝消息传递，减少内存复制开销
- **配置驱动**: JSON配置文件定义主题和订阅关系
- **消息队列**: 循环队列实现，支持消息缓冲和流量控制
- **端点管理**: 统一的端点注册、查找和管理机制
- **统计监控**: 通信统计和健康状态监控
- **接收延时控制**: 支持配置接收消息时的休眠延时，优化CPU使用率

## 系统要求

- **操作系统**: Linux
- **编译器**: 支持C99标准的GCC/Clang
- **构建工具**: CMake 3.10+
- **依赖库**: 
  - pthread (线程同步)
  - cJSON (JSON解析，已包含在项目中)

## 构建说明

### 基本构建

```bash
# 创建构建目录
mkdir build && cd build

# 配置项目（默认启用examples和tests）
cmake ..

# 编译项目
make -j$(nproc)
```

### 构建选项

本项目支持以下CMake构建选项，可以在cmake配置时指定：

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `BUILD_EXAMPLES` | `ON` | 是否编译示例程序 |
| `BUILD_TESTS` | `ON` | 是否编译单元测试 |

### 构建配置示例

#### 1. 默认构建（编译所有组件）
```bash
cmake ..
make -j$(nproc)
```
这将编译库文件、示例程序和单元测试。

#### 2. 仅编译库文件
```bash
cmake .. -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF
make -j$(nproc)
```
只编译核心库文件，不编译示例和测试。

#### 3. 编译库文件和示例程序
```bash
cmake .. -DBUILD_EXAMPLES=ON -DBUILD_TESTS=OFF
make -j$(nproc)
```
编译库文件和示例程序，不编译单元测试。

#### 4. 编译库文件和单元测试
```bash
cmake .. -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=ON
make -j$(nproc)
```
编译库文件和单元测试，不编译示例程序。

#### 5. 查看构建配置
```bash
cmake .. -DBUILD_EXAMPLES=ON -DBUILD_TESTS=ON
```
配置时会显示当前的构建选项状态：
```
-- Build configuration:
--   BUILD_EXAMPLES: ON
--   BUILD_TESTS:    ON
```

### 安装

```bash
# 在build目录中
make install
```

## 目录结构

```
amp-shm/
├── CMakeLists.txt              # 根项目CMake构建配置
├── README.md                   # 项目说明文档
├── include/                    # 公共头文件
│   ├── pubsub.h               # 发布订阅核心接口
│   ├── shm_comm.h             # 共享内存通信接口
│   ├── shm_queue.h            # 共享内存队列定义
│   ├── shm_endpoint.h         # 端点管理接口
│   ├── shm_errors.h           # 错误码定义
│   ├── shm_lock.h             # 进程间锁机制
│   ├── shm_log.h              # 日志系统接口
│   ├── json_config_processor.h # JSON配置处理模块
│   └── ...                    # 其他头文件
├── src/                       # 源代码实现
│   ├── pubsub.c               # 发布订阅实现
│   ├── shm_comm.c             # 共享内存通信实现
│   ├── shm_queue.c            # 共享内存队列实现
│   ├── shm_endpoint.c         # 端点管理实现
│   └── ...                    # 其他源文件
├── example/                   # 示例代码
│   ├── pub_test.c             # 发布者测试示例
│   ├── sub_test.c             # 订阅者测试示例
│   ├── sub_test2.c            # 订阅者测试示例2
│   ├── shm_pubsub_config.c    # 发布订阅配置示例
│   └── *.json                 # JSON配置文件
├── test/                      # 测试代码
│   └── unit/                  # 单元测试
│       ├── test_shm_comm.c    # 共享内存通信测试
│       ├── test_pubsub.c      # 发布订阅测试
│       └── ...                # 其他测试文件
├── doc/                       # 项目文档
│   ├── DESIGN.md              # 系统设计文档
│   └── CHANGE_LOG.md          # 更改日志
├── message/                   # 消息处理模块（第三方）
└── build/                     # 构建输出目录
```

## 使用示例

### 运行示例程序

#### 1. 启动配置进程（主进程）
```bash
cd build
./example/shm_pubsub_config
```

#### 2. 启动发布者
```bash
cd build
./example/pub_test
```

#### 3. 启动订阅者
```bash
# 终端1
cd build
./example/sub_test

# 终端2
cd build
./example/sub_test2
```

### 运行单元测试

```bash
cd build
# 运行所有单元测试
./bin/unit_tests

# 或使用CTest
ctest --output-on-failure
```

## 配置文件

项目使用JSON格式的配置文件来定义主题和订阅关系。示例配置文件位于`example/`目录：

- `pubsub_config.json` - 基础配置
- `pubsub_config2.json` - 扩展配置1
- `pubsub_config3.json` - 扩展配置2

配置文件格式示例：
```json
{
  "center": [
    {
      "topic": "example_topic",
      "len": 100,
      "topic_priority": 0,
      "msg_size": 1024,
      "subers": [
        {"name": "subscriber1"},
        {"name": "subscriber2"}
      ],
      "pubers": [
        {"name": "publisher1"},
        {"name": "publisher2"}
      ]
    }
  ]
}
```

## API接口

### 主要接口

- `pubsub_config_new_init()` - 初始化发布订阅配置对象
- `pubsub_config_new_deinit()` - 释放发布订阅配置对象
- `pub_init_config_files()` - 从配置文件初始化配置
- `pub_init_config_strs()` - 从JSON字符串初始化配置
- `pubsub_cal_mem()` - 计算所需共享内存大小
- `pubsub_init()` - 初始化发布订阅系统
- `pub_msg()` - 发布消息到指定主题
- `recv_msg()` - 接收消息（循环while 1，需要单独起线程运行）

### 使用示例

```c
#include "pubsub.h"

int main() {
    pubsub_config_new_t pcfg;
    
    // 初始化配置对象
    pubsub_config_new_init(&pcfg);
    
    // 从配置文件加载配置
    const char* config_files[] = {"config.json"};
    pub_init_config_files(&pcfg, config_files, 1);
    
    // 计算所需内存
    int mem_size = pubsub_cal_mem(&pcfg);
    
    // 创建共享内存
    int shmid = shmget(SHM_KEY, mem_size, IPC_CREAT | 0666);
    void* shm_addr = shmat(shmid, NULL, 0);
    
    // 初始化系统
    pubsub_init(&pcfg, 1, shm_addr, mem_size);
    
    // 发布消息
    const char* message = "Hello, World!";
    pub_msg(&pcfg, "publisher1", "topic1", message, strlen(message));
    
    // 接收消息
    recv_msg(&pcfg, "subscriber1");
    
    // 清理资源
    pubsub_config_new_deinit(&pcfg);
    return 0;
}
```

## 测试

### 单元测试

项目使用CuTest框架进行单元测试，覆盖以下模块：
- 共享内存通信模块 (shm_comm)
- 共享内存队列模块 (shm_queue)  
- 端点管理模块 (shm_endpoint)
- 发布订阅模块 (pubsub)
- JSON配置处理模块 (json_config_processor)

运行测试：
```bash
cd build
./bin/unit_tests
```

### 性能测试

项目包含性能测试示例：
- `test_perf.c` - 性能测试程序
- `shm_comm_perf_test.c/h` - 共享内存通信性能测试工具

## 文档

详细的设计文档和API说明请参考：
- `doc/DESIGN.md` - 系统设计文档
- `doc/CHANGE_LOG.md` - 更改日志
- 各模块头文件中的Doxygen注释

## 常见问题

### Q: 如何只编译库文件而不编译示例和测试？
A: 使用以下命令：
```bash
cmake .. -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF
make -j$(nproc)
```

### Q: 如何只运行单元测试？
A: 使用以下命令：
```bash
cmake .. -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=ON
make -j$(nproc)
./bin/unit_tests
```

### Q: 编译时出现错误怎么办？
A: 请检查：
1. 是否满足系统要求（Linux系统，CMake 3.10+）
2. 编译器是否支持C99标准
3. 依赖库是否正确安装
4. 查看详细的错误信息并参考文档

### Q: 如何修改配置文件？
A: 编辑`example/`目录下的JSON配置文件，然后重新编译运行示例程序。
