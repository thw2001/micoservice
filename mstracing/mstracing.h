#ifndef MSTRACING_H
#define MSTRACING_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "span_json_tools.h"
#include "span_context_json_tools.h"
#include "uuid4.h"
#include <time.h>

/*

                mstracing_new
                    |
                    v
      |         开启一个新的span
----> | 作为起始span               从其他span继承信息|
|        v                           v
|  mstracing_span_trace    mstracing_span_context_extract
|        |                           |
|        v                           v
|                    v
|            mstracing_start_trace
|                    |
|                    v
|    --------------------------------
|    | mstracing_set_tag
|    | mstracing_add_log_fields
|    | mstracing_set_baggage_item
|    |
|    |
|    --------------------------------
|                    |
|                    v
|            mstracing_finsh_trace
|                    |
|                    v
|    --------------------------------
|    | mstracing_get_span_str
|    | mstracing_span_context_inject
|    | mstracing_set_baggage_item
|    |
|    |
|    --------------------------------
|                    |
|                    v
------- mstracing_clear_current_tracing
                    |
                    v
            mstracing_delete
*/

// 定义一个结构体来存储时间信息
typedef struct {
    struct timespec start_time;
    struct timespec end_time;
} mstracing_timer;


typedef struct mstracing
{
    span span;
    span_context span_context;
    char* operationName;
    char* spanId;
    mstracing_timer timer;
}mstracing;

/**
 * @brief 创建tracing
 * 
 * @param operationName 
 * @return mstracing* 
 */
mstracing* mstracing_new(char* operationName);

/**
 * @brief 从上游继承span context作为tracing的一环
 * 
 * @param mst 
 * @param span_context_str 
 * @return int 
 */
int mstracing_span_context_extract(mstracing* mst, char* span_context_str);

/**
 * @brief 开启一个新的tracing
 * 
 * @param mst 
 * @return int 
 */
int mstracing_span_trace(mstracing* mst);

/**
 * @brief 启动tracing
 * 
 * @param mst 
 * @return int 
 */
int mstracing_start_trace(mstracing* mst);

int mstracing_set_tag(mstracing* mst);

int mstracing_add_log_fields(mstracing* mst);

int mstracing_set_baggage_item(mstracing* mst);

/**
 * @brief 停止tracing
 * 
 * @param mst 
 * @return int 
 */
int mstracing_finsh_trace(mstracing* mst);

/**
 * @brief 获取span的str
 * 
 * @param mst 
 * @return char* 
 */
char* mstracing_get_span_str(mstracing* mst);

/**
 * @brief 把span context压入str，用以传给下一个
 * 
 * @param mst 
 * @return char* 
 */
char* mstracing_span_context_inject(mstracing* mst);

void mstracing_clear_current_tracing(mstracing* mst);

void mstracing_delete(mstracing* mst);

#ifdef __cplusplus
}
#endif
#endif // MSTRACING_H