#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include "mstracing.h"
#include "cf_log.h"
#include <time.h>
#include <string.h>
#include "base_types.h"
#include "snow_flake.h"
#include "unistd.h"

#define TIME_STR_SIZE 32  // More than enough for millisecond string

/**
 * @brief 初始化计时器
 * 
 * @param timer 计时器指针
 */
void mstracing_timer_init(mstracing_timer *timer)
{

}

/**
 * @brief 启动计时器
 * 
 * @param timer 计时器指针
 */
void mstracing_timer_start(mstracing_timer *timer) {
    if (clock_gettime(CLOCK_REALTIME, &timer->start_time) == -1) {
        return;
    }
}

/**
 * @brief 停止计时器
 * 
 * @param timer 计时器指针
 */
void mstracing_timer_stop(mstracing_timer *timer) {
    if (clock_gettime(CLOCK_REALTIME, &timer->end_time) == -1) {
        return;
    }
}

/**
 * @brief 计算并返回执行时间（以毫秒为单位）
 * 
 * @param timer 计时器指针
 * @return long 执行时间（毫秒）
 */
long mstracing_timer_elapsed_ms(mstracing_timer *timer) {
    // Calculate the difference in seconds and nanoseconds
    long sec_diff = timer->end_time.tv_sec - timer->start_time.tv_sec;
    long nsec_diff = timer->end_time.tv_nsec - timer->start_time.tv_nsec;

    // Handle case where end->tv_nsec is less than start->tv_nsec
    if (nsec_diff < 0) {
        nsec_diff += 1000000000L; // Nanoseconds per second
        sec_diff -= 1;
    }

    // Convert the difference into milliseconds
    long ms_diff = (sec_diff * 1000L) + (nsec_diff / 1000000L);
    // long st_ms = timer->start_time.tv_nsec / 1000000, ed_ms = timer->end_time.tv_nsec / 1000000;
    return ms_diff;
}


/**
 * @brief 生成毫秒级时间戳字符串
 * 
 * @param str 输出字符串缓冲区
 * @param size 缓冲区大小
 * @return int 执行结果
 *         0: 成功
 *        -1: 失败
 */
int time_str_ms_iso8601(char* str, int size) {
    if (size < TIME_STR_SIZE) {  // 确保缓冲区足够大
        return -1;
    }

    // 获取当前时间（秒和纳秒）
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        return -1;
    }

    // 计算完整毫秒时间戳（秒*1000 + 纳秒/1000000）
    long long ms = (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000;

    // 格式化为毫秒时间戳字符串
    sprintf(str, "%lld", ms);

    return 0;
}


/**
 * @brief 创建新的 mstracing 对象
 * 
 * @param operationName 操作名称
 * @return mstracing* 新创建的 mstracing 对象指针
 */
mstracing *mstracing_new(char* operationName)
{
    mstracing * mst = malloc(sizeof(mstracing));
    span_init(&mst->span);
    // span_context_init(&mst->recv);
    span_context_init(&mst->span_context);
    mstracing_timer_init(&mst->timer);

    char* uuid_str1 = id_generate_string();
    mst->operationName = strdup(operationName);
    mst->spanId = uuid_str1;

    return mst;
}

/**
 * @brief 生成跟踪信息
 * 
 * @param mst mstracing 对象指针
 * @return int 执行结果
 *         BASE_SUCCESS: 成功
 *         BASE_FAILED: 失败
 */
int mstracing_span_trace(mstracing* mst)
{
    UUID4_STATE_T state;
    UUID4_PREFIX(seed)(&state);
    UUID4_T uuid4;
    UUID4_PREFIX(gen)(&state, &uuid4);

    char* uuid_str = malloc(sizeof(char) * (UUID4_STR_BUFFER_SIZE + 1));
    UUID4_PREFIX(to_s)(uuid4, uuid_str, (UUID4_STR_BUFFER_SIZE + 1));
    mst->span.traceId = uuid_str;
    mst->span_context.traceId = strdup(uuid_str);
    mst->span.spanId = strdup(mst->spanId);
    mst->span.operationName = strdup(mst->operationName);

    mst->span_context.spanId = strdup(mst->spanId);
}

/**
 * @brief 从字符串中提取 span 上下文信息
 * 
 * @param mst mstracing 对象指针
 * @param span_context_str span 上下文字符串
 * @return int 执行结果
 *         BASE_SUCCESS: 成功
 *         BASE_FAILED: 失败
 */
int mstracing_span_context_extract(mstracing* mst, char* span_context_str)
{
    int ret = 0;
    span_context recv;
    span_context_init(&recv);
    if (BASE_SUCCESS != (ret = span_context_deserialize_str(&recv, span_context_str)))
    {
        log_error("%d", ret);
        // return BASE_FAILED;
    }
    log_debug("%s %s %s", recv.spanId, recv.traceId);
    mst->span.parentSpanId = strdup(recv.spanId);
    mst->span.traceId = strdup(recv.traceId);
    mst->span_context.traceId = strdup(recv.traceId);
    mst->span.spanId = strdup(mst->spanId);
    mst->span.operationName = strdup(mst->operationName);
    mst->span_context.spanId = strdup(mst->spanId);

    span_context_deinit(&recv);
    return ret;
}

/**
 * @brief 开始跟踪
 * 
 * @param mst mstracing 对象指针
 * @return int 执行结果
 *         BASE_SUCCESS: 成功
 *         BASE_FAILED: 失败
 */
int mstracing_start_trace(mstracing* mst)
{
    mstracing_timer_start(&mst->timer);
    mst->span.startTime = malloc(sizeof(char)*(TIME_STR_SIZE) + 1);
    time_str_ms_iso8601(mst->span.startTime, TIME_STR_SIZE);
}

/**
 * @brief 将 span 上下文信息序列化为字符串
 * 
 * @param mst mstracing 对象指针
 * @return char* 序列化后的字符串
 */
char* mstracing_span_context_inject(mstracing* mst)
{
    return span_context_to_str(&mst->span_context);
}

/**
 * @brief 结束跟踪
 * 
 * @param mst mstracing 对象指针
 * @return int 执行结果
 *         BASE_SUCCESS: 成功
 *         BASE_FAILED: 失败
 */
int mstracing_finsh_trace(mstracing* mst)
{
    mstracing_timer_stop(&mst->timer);
    mst->span.duration = mstracing_timer_elapsed_ms(&mst->timer);

    mst->span.endTime = malloc(sizeof(char)*(TIME_STR_SIZE) + 1);
    time_str_ms_iso8601(mst->span.endTime, TIME_STR_SIZE);
}

/**
 * @brief 获取 span 信息的字符串表示
 * 
 * @param mst mstracing 对象指针
 * @return char* span 信息字符串
 */
char* mstracing_get_span_str(mstracing* mst)
{
    return span_to_str(&mst->span);
}

/**
 * @brief 清除当前跟踪信息
 * 
 * @param mst mstracing 对象指针
 */
void mstracing_clear_current_tracing(mstracing* mst)
{
    span_deinit(&mst->span);
    span_context_deinit(&mst->span_context);
}

/**
 * @brief 删除 mstracing 对象
 * 
 * @param mst mstracing 对象指针
 */
void mstracing_delete(mstracing *mst)
{
    if (NULL == mst)
    {
        return;
    }
    free(mst->operationName);
    free(mst->spanId);
    free(mst);
}
