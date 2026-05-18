#ifndef COLLECTOR_H
#define COLLECTOR_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "mstracing.h"
#include "span_json_tools.h"
#include "map.h"
#include "abs_comm.h"

typedef map_t(span) map_span_t;

typedef struct tracing_log
{
    map_span_t span_cache;
}tracing_log;

typedef map_t(tracing_log) map_tracing_log_t;

typedef struct collector
{
    /* data */
    map_tracing_log_t m_trace_log;
    abs_comm* rs;
}collector;

collector* collector_new(abs_comm_connect* rsct);
void collector_delete(collector* c);
void collector_loop(collector* c);
char* collector_serialize_trace_log(collector* c);

#ifdef __cplusplus
}
#endif
#endif // COLLECTOR_H
