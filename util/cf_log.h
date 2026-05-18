#ifndef _CF_LOG_H
#define _CF_LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

#define LOG_VERSION "0.1.0"

typedef struct
{
    va_list ap;
    const char *fmt;
    const char *file;
    struct tm *time;
    void *udata;
    int line;
    int level;
} log_Event;

static const char *level_strings[] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

#ifdef LOG_USE_COLOR
static const char *level_colors[] = {
    "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"};
#endif

typedef void (*log_LogFn)(log_Event *ev);
typedef void (*log_LockFn)(bool lock, void *udata);

enum
{
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
};

#define log_trace(...) log_log(LOG_TRACE, __FUNCTION__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_log(LOG_DEBUG, __FUNCTION__, __LINE__, __VA_ARGS__)
#define log_info(...) log_log(LOG_INFO, __FUNCTION__, __LINE__, __VA_ARGS__)
#define log_warn(...) log_log(LOG_WARN, __FUNCTION__, __LINE__, __VA_ARGS__)
#define log_error(...) log_log(LOG_ERROR, __FUNCTION__, __LINE__, __VA_ARGS__)
#define log_fatal(...) log_log(LOG_FATAL, __FUNCTION__, __LINE__, __VA_ARGS__)

const char *log_level_string(int level);
void log_set_lock(log_LockFn fn, void *udata);
void log_set_level(int level);
void log_set_quiet(bool enable);
int log_add_callback(log_LogFn fn, void *udata, int level);
int log_add_fp(FILE *fp, int level);

void log_log(int level, const char *file, int line, const char *fmt, ...);

// void log_sem_lock(bool lock, void* udata);
void log_mutex_lock(bool lock, void *udata);

#endif
