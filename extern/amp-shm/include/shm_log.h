#ifndef SHM_LOG_H
#define SHM_LOG_H
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <time.h>

// =================== 日志系统配置 ===================
// 编译时日志级别 (0:DEBUG, 1:INFO, 2:WARNING, 3:ERROR, 4:FATAL, 5:NONE)
#ifndef COMPILE_LOG_LEVEL
#define COMPILE_LOG_LEVEL 0  // 默认编译所有日志
#endif

// 日志级别枚举
typedef enum {
    LOG_LEVEL_TRACE   = 0,
    LOG_LEVEL_DEBUG   = 1,
    LOG_LEVEL_INFO    = 2,
    LOG_LEVEL_WARNING = 3,
    LOG_LEVEL_ERROR   = 4,
    LOG_LEVEL_FATAL   = 5
} LogLevel;

// 全局运行时日志级别 (可动态修改)
static LogLevel runtime_log_level = LOG_LEVEL_INFO;

// =================== 核心日志宏实现 ===================
#define LOG(level, level_str, fmt, ...) do { \
    if (level >= runtime_log_level && level >= COMPILE_LOG_LEVEL) { \
        printf("%-5s %s:%d: " fmt "\n", \
               level_str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    } \
} while (0)

// =================== 分级日志宏 ===================
#define log_trace(fmt, ...)   LOG(LOG_LEVEL_DEBUG,   "TRACE",   fmt, ##__VA_ARGS__)
#define log_debug(fmt, ...)   LOG(LOG_LEVEL_DEBUG,   "DEBUG",   fmt, ##__VA_ARGS__)
#define log_info(fmt, ...)    LOG(LOG_LEVEL_INFO,    "INFO",    fmt, ##__VA_ARGS__)
#define log_warn(fmt, ...) LOG(LOG_LEVEL_WARNING, "WARN", fmt, ##__VA_ARGS__)
#define log_error(fmt, ...)   LOG(LOG_LEVEL_ERROR,   "ERROR",   fmt, ##__VA_ARGS__)
#define log_fatal(fmt, ...)   LOG(LOG_LEVEL_FATAL,   "FATAL",   fmt, ##__VA_ARGS__)


#ifdef __cplusplus
}
#endif
#endif // SHM_LOG_H