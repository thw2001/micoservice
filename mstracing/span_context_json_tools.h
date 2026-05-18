/*
 * 本文件由 json_to_c_struct.py 自动生成
 * span_context_json_tools.h
 * 生成时间: 2024-12-02 14:08:35
 * 如无必要，请勿手动修改此文件
 */

#ifndef SPAN_CONTEXT_JSON_TOOLS_H
#define SPAN_CONTEXT_JSON_TOOLS_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "cJSON.h"
#include "vec.h"
#include "stdio.h"

typedef struct span_context_baggage{
    char* user_id;
    char* session_id;
} span_context_baggage;


typedef struct span_context{
    span_context_baggage baggage;
    char* spanId;
    char* traceId;
    int sampled;
} span_context;


void span_context_serialize(span_context* obj, cJSON* json);

int span_context_deserialize(cJSON* json, span_context* obj);

int span_context_deserialize_str(span_context* obj, char* json_str);

int span_context_deserialize_file(span_context* obj, char* file_path);

void span_context_init(span_context* obj);

void span_context_deinit(span_context* obj);

char* span_context_to_str(span_context* obj);

#ifdef __cplusplus
}
#endif
#endif
