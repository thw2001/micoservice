/*
 * 本文件由 json_to_c_struct.py 自动生成
 * span_json_tools.h
 * 生成时间: 2024-12-02 14:08:35
 * 如无必要，请勿手动修改此文件
 */

#ifndef SPAN_JSON_TOOLS_H
#define SPAN_JSON_TOOLS_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "cJSON.h"
#include "vec.h"
#include "stdio.h"

typedef struct span_logs_fields{
    char* message;
    char* event;
} span_logs_fields;


typedef struct span_logs{
    char* timestamp;
    span_logs_fields fields;
} span_logs;


typedef vec_t(span_logs) vec_span_logs;

typedef struct span_tags{
    char* arg1;
    char* arg2;
} span_tags;


typedef struct span_warnings{
    char* timestamp;
    char* message;
} span_warnings;


typedef vec_t(span_warnings) vec_span_warnings;

typedef struct span_references{
    char* refType;
    char* spanId;
    char* traceId;
} span_references;


typedef vec_t(span_references) vec_span_references;

typedef struct span_errors_details_inputData{
    char* data;
} span_errors_details_inputData;


typedef struct span_errors_details{
    span_errors_details_inputData inputData;
} span_errors_details;


typedef struct span_errors{
    char* timestamp;
    char* message;
    span_errors_details details;
    char* stackTrace;
} span_errors;


typedef vec_t(span_errors) vec_span_errors;

typedef struct span{
    char* traceId;
    vec_span_logs logs;
    span_tags tags;
    char* operationName;
    vec_span_warnings warnings;
    char* parentSpanId;
    vec_span_references references;
    char* startTime;
    int duration;
    char* spanId;
    vec_span_errors errors;
    char* endTime;
} span;


void span_serialize(span* obj, cJSON* json);

int span_deserialize(cJSON* json, span* obj);

int span_deserialize_str(span* obj, char* json_str);

int span_deserialize_file(span* obj, char* file_path);

void span_init(span* obj);

void span_deinit(span* obj);

char* span_to_str(span* obj);

#ifdef __cplusplus
}
#endif
#endif
