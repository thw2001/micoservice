/*
 * 本文件由 json_to_c_struct.py 自动生成
 * span_context_json_tools.c
 * 生成时间: 2024-12-02 14:08:35
 * 如无必要，请勿手动修改此文件
 */

#include "span_context_json_tools.h"

void span_context_baggage_init(span_context_baggage* obj) {
    obj->user_id = NULL;
    obj->session_id = NULL;
}

void span_context_init(span_context* obj) {
    span_context_baggage_init(&obj->baggage);
    obj->spanId = NULL;
    obj->traceId = NULL;
    obj->sampled = 0;
}


void span_context_baggage_deinit(span_context_baggage* obj) {
    if (NULL != obj->user_id)
        free(obj->user_id);
    if (NULL != obj->session_id)
        free(obj->session_id);
}

void span_context_deinit(span_context* obj) {
    span_context_baggage_deinit(&obj->baggage);
    if (NULL != obj->spanId)
        free(obj->spanId);
    if (NULL != obj->traceId)
        free(obj->traceId);
}


void span_context_baggage_serialize(span_context_baggage* obj, cJSON* json) {
    cJSON_AddItemToObject(json, "user_id", cJSON_CreateString(obj->user_id));
    cJSON_AddItemToObject(json, "session_id", cJSON_CreateString(obj->session_id));
}

void span_context_serialize(span_context* obj, cJSON* json) {
    cJSON *cjson_obj_span_context_baggage = cJSON_CreateObject();
    span_context_baggage_serialize(&obj->baggage, cjson_obj_span_context_baggage);
    cJSON_AddItemToObject(json, "baggage", cjson_obj_span_context_baggage);
    cJSON_AddItemToObject(json, "spanId", cJSON_CreateString(obj->spanId));
    cJSON_AddItemToObject(json, "traceId", cJSON_CreateString(obj->traceId));
    cJSON_AddItemToObject(json, "sampled", cJSON_CreateNumber(obj->sampled));
}


int span_context_baggage_deserialize(cJSON* json, span_context_baggage* obj) {
    int result = 0;
    cJSON* cjson_obj_user_id = cJSON_GetObjectItemCaseSensitive(json, "user_id");
    if (cjson_obj_user_id) {
        obj->user_id = strdup(cJSON_GetStringValue(cjson_obj_user_id));
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_session_id = cJSON_GetObjectItemCaseSensitive(json, "session_id");
    if (cjson_obj_session_id) {
        obj->session_id = strdup(cJSON_GetStringValue(cjson_obj_session_id));
    }
    else {
        result += -1;
    }
    return result;

}

int span_context_deserialize(cJSON* json, span_context* obj) {
    int result = 0;
    span_context_baggage_init(&obj->baggage);
    cJSON* cjson_obj_baggage = cJSON_GetObjectItemCaseSensitive(json, "baggage");
    if (cjson_obj_baggage) {
        result += span_context_baggage_deserialize(cjson_obj_baggage, &obj->baggage);
    } else {
        result += -1;
    }
    cJSON* cjson_obj_spanId = cJSON_GetObjectItemCaseSensitive(json, "spanId");
    if (cjson_obj_spanId) {
        obj->spanId = strdup(cJSON_GetStringValue(cjson_obj_spanId));
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_traceId = cJSON_GetObjectItemCaseSensitive(json, "traceId");
    if (cjson_obj_traceId) {
        obj->traceId = strdup(cJSON_GetStringValue(cjson_obj_traceId));
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_sampled = cJSON_GetObjectItemCaseSensitive(json, "sampled");
    if (cjson_obj_sampled) {
        obj->sampled = cJSON_GetNumberValue(cjson_obj_sampled);
    }
    else {
        result += -1;
    }
    return result;

}


int span_context_deserialize_str(span_context* obj, char* json_str) {
    cJSON *json_parsed = cJSON_Parse(json_str);
    int result = span_context_deserialize(json_parsed, obj);
    cJSON_Delete(json_parsed);
    return result;
}

int span_context_deserialize_file(span_context* obj, char* file_path) {
    char* jsonStr;
    int result = -1;
    FILE *pFile = fopen(file_path, "r");
    if (pFile) {
        fseek(pFile, 0L, SEEK_END);
        long lenFile = ftell(pFile);
        fseek(pFile, 0L, SEEK_SET);
        jsonStr = malloc(sizeof(char) * (lenFile + 1));
        jsonStr[fread(jsonStr, 1, lenFile, pFile)] = '\0';
        result = span_context_deserialize_str(obj, jsonStr);
        free(jsonStr);
        fclose(pFile);
    }
    return result;
}

char* span_context_to_str(span_context* obj) {
    cJSON* cjson = cJSON_CreateObject();
    span_context_serialize(obj, cjson);
    char* tmp = cJSON_Print(cjson);
    cJSON_Delete(cjson);
    return tmp;
}

