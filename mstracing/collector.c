#include "collector.h"
#include "abs_comm.h"
#include "comm_fan_in_out.h"
#include "cf_log.h"
#include "base_types.h"
#include "unistd.h"

int recv_msg_from_agent(void* arg, void* msg, uint32_t len)
{
    collector* c = arg;
    // log_debug("%s", (char*)msg);
    span tmp_span;
    span_init(&tmp_span);
    int ret = 0;
    if (BASE_SUCCESS != (ret = span_deserialize_str(&tmp_span, (char*)msg)))
    {
        log_error("span str error num : %d", ret);
    }
    if (NULL == tmp_span.traceId || NULL == tmp_span.spanId)
    {
        log_error("traceId spanId not exist!");
        span_deinit(&tmp_span);
        return BASE_FAILED;
    }
    

    tracing_log* tl = map_get(&c->m_trace_log, tmp_span.traceId);
    if (tl)
    {
        map_set(&tl->span_cache, tmp_span.traceId, tmp_span);
    }
    else
    {
        tracing_log new_tl;
        map_init(&new_tl.span_cache);
        map_set(&new_tl.span_cache, tmp_span.spanId, tmp_span);
        map_set(&c->m_trace_log, tmp_span.traceId, new_tl);
    }
}

collector *collector_new(abs_comm_connect* rsct)
{
    collector* c = malloc(sizeof(collector));
    // abs_comm_connect rsct = {.ip = "127.0.0.1", .port = 19999};
    c->rs = abs_comm_new("collector", rsct);
    abs_comm_sub(c->rs, COLLECTOR_ID, c, recv_msg_from_agent);
    map_init(&c->m_trace_log);
    return c;
}

void collector_loop(collector* c)
{
    int line = 0;
    while (1)
    {
        sleep(4);
        char* tmp = collector_serialize_trace_log(c);
        log_debug("%s", tmp);
        free(tmp);
    }
}

void collector_delete(collector* c)
{
    if (!c) return;

    // Free all spans in span_cache maps
    const char* trace_id;
    map_iter_t trace_iter = map_iter(&c->m_trace_log);
    
    while ((trace_id = map_next(&c->m_trace_log, &trace_iter))) {
        tracing_log* tl = map_get(&c->m_trace_log, trace_id);
        
        const char* span_id;
        map_iter_t span_iter = map_iter(&tl->span_cache);
        
        while ((span_id = map_next(&tl->span_cache, &span_iter))) {
            span* s = map_get(&tl->span_cache, span_id);
            span_deinit(s);
        }
        map_deinit(&tl->span_cache);
    }
    map_deinit(&c->m_trace_log);

    // Free registry server
    if (c->rs) {
        abs_comm_delete(c->rs);
    }

    free(c);
}

char* collector_serialize_trace_log(collector* c)
{
    cJSON* root = cJSON_CreateObject();
    if (!root) return NULL;

    const char* trace_id;
    map_iter_t trace_iter = map_iter(&c->m_trace_log);
    
    while ((trace_id = map_next(&c->m_trace_log, &trace_iter))) {
        tracing_log* tl = map_get(&c->m_trace_log, trace_id);
        cJSON* spans_array = cJSON_AddArrayToObject(root, trace_id);
        if (!spans_array) continue;

        const char* span_id;
        map_iter_t span_iter = map_iter(&tl->span_cache);
        
        while ((span_id = map_next(&tl->span_cache, &span_iter))) {
            span* s = map_get(&tl->span_cache, span_id);
            cJSON* span_json = cJSON_CreateObject();
            if (span_json) {
                span_serialize(s, span_json);
                cJSON_AddItemToArray(spans_array, span_json);
            }
        }
    }
    char* json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_str;
}
