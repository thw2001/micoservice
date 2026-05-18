/*
 * 本文件由 json_to_c_struct.py 自动生成
 * span_json_tools.c
 * 生成时间: 2024-12-02 14:08:35
 * 如无必要，请勿手动修改此文件
 */

#include "span_json_tools.h"

void span_logs_fields_init(span_logs_fields* obj) {
    obj->message = NULL;
    obj->event = NULL;
}


void span_logs_init(span_logs* obj) {
    obj->timestamp = NULL;
    span_logs_fields_init(&obj->fields);
}


void span_tags_init(span_tags* obj) {
    obj->arg1 = NULL;
    obj->arg2 = NULL;
}


void span_warnings_init(span_warnings* obj) {
    obj->timestamp = NULL;
    obj->message = NULL;
}


void span_references_init(span_references* obj) {
    obj->refType = NULL;
    obj->spanId = NULL;
    obj->traceId = NULL;
}


void span_errors_details_inputData_init(span_errors_details_inputData* obj) {
    obj->data = NULL;
}


void span_errors_details_init(span_errors_details* obj) {
    span_errors_details_inputData_init(&obj->inputData);
}


void span_errors_init(span_errors* obj) {
    obj->timestamp = NULL;
    obj->message = NULL;
    span_errors_details_init(&obj->details);
    obj->stackTrace = NULL;
}

void span_init(span* obj) {
    obj->traceId = NULL;
    vec_init(&obj->logs);
    span_tags_init(&obj->tags);
    obj->operationName = NULL;
    vec_init(&obj->warnings);
    obj->parentSpanId = NULL;
    vec_init(&obj->references);
    obj->startTime = NULL;
    obj->duration = 0;
    obj->spanId = NULL;
    vec_init(&obj->errors);
    obj->endTime = NULL;
}


void span_logs_fields_deinit(span_logs_fields* obj) {
    if (NULL != obj->message)
        free(obj->message);
    if (NULL != obj->event)
        free(obj->event);
}


void span_logs_deinit(span_logs* obj) {
    if (NULL != obj->timestamp)
        free(obj->timestamp);
    span_logs_fields_deinit(&obj->fields);
}


void span_tags_deinit(span_tags* obj) {
    if (NULL != obj->arg1)
        free(obj->arg1);
    if (NULL != obj->arg2)
        free(obj->arg2);
}


void span_warnings_deinit(span_warnings* obj) {
    if (NULL != obj->timestamp)
        free(obj->timestamp);
    if (NULL != obj->message)
        free(obj->message);
}


void span_references_deinit(span_references* obj) {
    if (NULL != obj->refType)
        free(obj->refType);
    if (NULL != obj->spanId)
        free(obj->spanId);
    if (NULL != obj->traceId)
        free(obj->traceId);
}


void span_errors_details_inputData_deinit(span_errors_details_inputData* obj) {
    if (NULL != obj->data)
        free(obj->data);
}


void span_errors_details_deinit(span_errors_details* obj) {
    span_errors_details_inputData_deinit(&obj->inputData);
}


void span_errors_deinit(span_errors* obj) {
    if (NULL != obj->timestamp)
        free(obj->timestamp);
    if (NULL != obj->message)
        free(obj->message);
    span_errors_details_deinit(&obj->details);
    if (NULL != obj->stackTrace)
        free(obj->stackTrace);
}

void span_deinit(span* obj) {
    if (NULL != obj->traceId)
        free(obj->traceId);
    int span_logs_i = 0;
    span_logs* span_logs_iter;
    vec_foreach_ptr(&obj->logs, span_logs_iter, span_logs_i) {
        span_logs_deinit(span_logs_iter);
    }
    vec_deinit(&obj->logs);
    span_tags_deinit(&obj->tags);
    if (NULL != obj->operationName)
        free(obj->operationName);
    int span_warnings_i = 0;
    span_warnings* span_warnings_iter;
    vec_foreach_ptr(&obj->warnings, span_warnings_iter, span_warnings_i) {
        span_warnings_deinit(span_warnings_iter);
    }
    vec_deinit(&obj->warnings);
    if (NULL != obj->parentSpanId)
        free(obj->parentSpanId);
    int span_references_i = 0;
    span_references* span_references_iter;
    vec_foreach_ptr(&obj->references, span_references_iter, span_references_i) {
        span_references_deinit(span_references_iter);
    }
    vec_deinit(&obj->references);
    if (NULL != obj->startTime)
        free(obj->startTime);
    if (NULL != obj->spanId)
        free(obj->spanId);
    int span_errors_i = 0;
    span_errors* span_errors_iter;
    vec_foreach_ptr(&obj->errors, span_errors_iter, span_errors_i) {
        span_errors_deinit(span_errors_iter);
    }
    vec_deinit(&obj->errors);
    if (NULL != obj->endTime)
        free(obj->endTime);
}


void span_logs_fields_serialize(span_logs_fields* obj, cJSON* json) {
    cJSON_AddItemToObject(json, "message", cJSON_CreateString(obj->message));
    cJSON_AddItemToObject(json, "event", cJSON_CreateString(obj->event));
}


void span_logs_serialize(span_logs* obj, cJSON* json) {
    cJSON_AddItemToObject(json, "timestamp", cJSON_CreateString(obj->timestamp));
    cJSON *cjson_obj_span_logs_fields = cJSON_CreateObject();
    span_logs_fields_serialize(&obj->fields, cjson_obj_span_logs_fields);
    cJSON_AddItemToObject(json, "fields", cjson_obj_span_logs_fields);
}


void span_tags_serialize(span_tags* obj, cJSON* json) {
    cJSON_AddItemToObject(json, "arg1", cJSON_CreateString(obj->arg1));
    cJSON_AddItemToObject(json, "arg2", cJSON_CreateString(obj->arg2));
}


void span_warnings_serialize(span_warnings* obj, cJSON* json) {
    cJSON_AddItemToObject(json, "timestamp", cJSON_CreateString(obj->timestamp));
    cJSON_AddItemToObject(json, "message", cJSON_CreateString(obj->message));
}


void span_references_serialize(span_references* obj, cJSON* json) {
    cJSON_AddItemToObject(json, "refType", cJSON_CreateString(obj->refType));
    cJSON_AddItemToObject(json, "spanId", cJSON_CreateString(obj->spanId));
    cJSON_AddItemToObject(json, "traceId", cJSON_CreateString(obj->traceId));
}


void span_errors_details_inputData_serialize(span_errors_details_inputData* obj, cJSON* json) {
    cJSON_AddItemToObject(json, "data", cJSON_CreateString(obj->data));
}


void span_errors_details_serialize(span_errors_details* obj, cJSON* json) {
    cJSON *cjson_obj_span_errors_details_inputData = cJSON_CreateObject();
    span_errors_details_inputData_serialize(&obj->inputData, cjson_obj_span_errors_details_inputData);
    cJSON_AddItemToObject(json, "inputData", cjson_obj_span_errors_details_inputData);
}


void span_errors_serialize(span_errors* obj, cJSON* json) {
    cJSON_AddItemToObject(json, "timestamp", cJSON_CreateString(obj->timestamp));
    cJSON_AddItemToObject(json, "message", cJSON_CreateString(obj->message));
    cJSON *cjson_obj_span_errors_details = cJSON_CreateObject();
    span_errors_details_serialize(&obj->details, cjson_obj_span_errors_details);
    cJSON_AddItemToObject(json, "details", cjson_obj_span_errors_details);
    cJSON_AddItemToObject(json, "stackTrace", cJSON_CreateString(obj->stackTrace));
}

void span_serialize(span* obj, cJSON* json) {
    cJSON_AddItemToObject(json, "traceId", cJSON_CreateString(obj->traceId));
    cJSON* cjson_obj_vec_logs = cJSON_CreateArray();
    int span_logs_i = 0;
    span_logs* span_logs_iter;
    vec_foreach_ptr(&obj->logs, span_logs_iter, span_logs_i) {
        cJSON* tmp_obj = cJSON_CreateObject();
        span_logs_serialize(span_logs_iter, tmp_obj);
        cJSON_AddItemToArray(cjson_obj_vec_logs, tmp_obj);
    }
    cJSON_AddItemToObject(json, "logs", cjson_obj_vec_logs);
    cJSON *cjson_obj_span_tags = cJSON_CreateObject();
    span_tags_serialize(&obj->tags, cjson_obj_span_tags);
    cJSON_AddItemToObject(json, "tags", cjson_obj_span_tags);
    cJSON_AddItemToObject(json, "operationName", cJSON_CreateString(obj->operationName));
    cJSON* cjson_obj_vec_warnings = cJSON_CreateArray();
    int span_warnings_i = 0;
    span_warnings* span_warnings_iter;
    vec_foreach_ptr(&obj->warnings, span_warnings_iter, span_warnings_i) {
        cJSON* tmp_obj = cJSON_CreateObject();
        span_warnings_serialize(span_warnings_iter, tmp_obj);
        cJSON_AddItemToArray(cjson_obj_vec_warnings, tmp_obj);
    }
    cJSON_AddItemToObject(json, "warnings", cjson_obj_vec_warnings);
    cJSON_AddItemToObject(json, "parentSpanId", cJSON_CreateString(obj->parentSpanId));
    cJSON* cjson_obj_vec_references = cJSON_CreateArray();
    int span_references_i = 0;
    span_references* span_references_iter;
    vec_foreach_ptr(&obj->references, span_references_iter, span_references_i) {
        cJSON* tmp_obj = cJSON_CreateObject();
        span_references_serialize(span_references_iter, tmp_obj);
        cJSON_AddItemToArray(cjson_obj_vec_references, tmp_obj);
    }
    cJSON_AddItemToObject(json, "references", cjson_obj_vec_references);
    cJSON_AddItemToObject(json, "startTime", cJSON_CreateString(obj->startTime));
    cJSON_AddItemToObject(json, "duration", cJSON_CreateNumber(obj->duration));
    cJSON_AddItemToObject(json, "spanId", cJSON_CreateString(obj->spanId));
    cJSON* cjson_obj_vec_errors = cJSON_CreateArray();
    int span_errors_i = 0;
    span_errors* span_errors_iter;
    vec_foreach_ptr(&obj->errors, span_errors_iter, span_errors_i) {
        cJSON* tmp_obj = cJSON_CreateObject();
        span_errors_serialize(span_errors_iter, tmp_obj);
        cJSON_AddItemToArray(cjson_obj_vec_errors, tmp_obj);
    }
    cJSON_AddItemToObject(json, "errors", cjson_obj_vec_errors);
    cJSON_AddItemToObject(json, "endTime", cJSON_CreateString(obj->endTime));
}


int span_logs_fields_deserialize(cJSON* json, span_logs_fields* obj) {
    int result = 0;
    cJSON* cjson_obj_message = cJSON_GetObjectItemCaseSensitive(json, "message");
    if (cjson_obj_message) {
        obj->message = strdup(cJSON_GetStringValue(cjson_obj_message));
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_event = cJSON_GetObjectItemCaseSensitive(json, "event");
    if (cjson_obj_event) {
        obj->event = strdup(cJSON_GetStringValue(cjson_obj_event));
    }
    else {
        result += -1;
    }
    return result;

}


int span_logs_deserialize(cJSON* json, span_logs* obj) {
    int result = 0;
    cJSON* cjson_obj_timestamp = cJSON_GetObjectItemCaseSensitive(json, "timestamp");
    if (cjson_obj_timestamp) {
        obj->timestamp = strdup(cJSON_GetStringValue(cjson_obj_timestamp));
    }
    else {
        result += -1;
    }
    span_logs_fields_init(&obj->fields);
    cJSON* cjson_obj_fields = cJSON_GetObjectItemCaseSensitive(json, "fields");
    if (cjson_obj_fields) {
        result += span_logs_fields_deserialize(cjson_obj_fields, &obj->fields);
    } else {
        result += -1;
    }
    return result;

}


int span_tags_deserialize(cJSON* json, span_tags* obj) {
    int result = 0;
    cJSON* cjson_obj_arg1 = cJSON_GetObjectItemCaseSensitive(json, "arg1");
    if (cjson_obj_arg1) {
        obj->arg1 = strdup(cJSON_GetStringValue(cjson_obj_arg1));
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_arg2 = cJSON_GetObjectItemCaseSensitive(json, "arg2");
    if (cjson_obj_arg2) {
        obj->arg2 = strdup(cJSON_GetStringValue(cjson_obj_arg2));
    }
    else {
        result += -1;
    }
    return result;

}


int span_warnings_deserialize(cJSON* json, span_warnings* obj) {
    int result = 0;
    cJSON* cjson_obj_timestamp = cJSON_GetObjectItemCaseSensitive(json, "timestamp");
    if (cjson_obj_timestamp) {
        obj->timestamp = strdup(cJSON_GetStringValue(cjson_obj_timestamp));
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_message = cJSON_GetObjectItemCaseSensitive(json, "message");
    if (cjson_obj_message) {
        obj->message = strdup(cJSON_GetStringValue(cjson_obj_message));
    }
    else {
        result += -1;
    }
    return result;

}


int span_references_deserialize(cJSON* json, span_references* obj) {
    int result = 0;
    cJSON* cjson_obj_refType = cJSON_GetObjectItemCaseSensitive(json, "refType");
    if (cjson_obj_refType) {
        obj->refType = strdup(cJSON_GetStringValue(cjson_obj_refType));
    }
    else {
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
    return result;

}


int span_errors_details_inputData_deserialize(cJSON* json, span_errors_details_inputData* obj) {
    int result = 0;
    cJSON* cjson_obj_data = cJSON_GetObjectItemCaseSensitive(json, "data");
    if (cjson_obj_data) {
        obj->data = strdup(cJSON_GetStringValue(cjson_obj_data));
    }
    else {
        result += -1;
    }
    return result;

}


int span_errors_details_deserialize(cJSON* json, span_errors_details* obj) {
    int result = 0;
    span_errors_details_inputData_init(&obj->inputData);
    cJSON* cjson_obj_inputData = cJSON_GetObjectItemCaseSensitive(json, "inputData");
    if (cjson_obj_inputData) {
        result += span_errors_details_inputData_deserialize(cjson_obj_inputData, &obj->inputData);
    } else {
        result += -1;
    }
    return result;

}


int span_errors_deserialize(cJSON* json, span_errors* obj) {
    int result = 0;
    cJSON* cjson_obj_timestamp = cJSON_GetObjectItemCaseSensitive(json, "timestamp");
    if (cjson_obj_timestamp) {
        obj->timestamp = strdup(cJSON_GetStringValue(cjson_obj_timestamp));
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_message = cJSON_GetObjectItemCaseSensitive(json, "message");
    if (cjson_obj_message) {
        obj->message = strdup(cJSON_GetStringValue(cjson_obj_message));
    }
    else {
        result += -1;
    }
    span_errors_details_init(&obj->details);
    cJSON* cjson_obj_details = cJSON_GetObjectItemCaseSensitive(json, "details");
    if (cjson_obj_details) {
        result += span_errors_details_deserialize(cjson_obj_details, &obj->details);
    } else {
        result += -1;
    }
    cJSON* cjson_obj_stackTrace = cJSON_GetObjectItemCaseSensitive(json, "stackTrace");
    if (cjson_obj_stackTrace) {
        obj->stackTrace = strdup(cJSON_GetStringValue(cjson_obj_stackTrace));
    }
    else {
        result += -1;
    }
    return result;

}

int span_deserialize(cJSON* json, span* obj) {
    int result = 0;
    cJSON* cjson_obj_traceId = cJSON_GetObjectItemCaseSensitive(json, "traceId");
    if (cjson_obj_traceId) {
        obj->traceId = strdup(cJSON_GetStringValue(cjson_obj_traceId));
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_vec_logs = cJSON_GetObjectItemCaseSensitive(json, "logs");
    if (cjson_obj_vec_logs) {
        cJSON* item;
        cJSON_ArrayForEach(item, cjson_obj_vec_logs) {
            span_logs span_logs_item;
            span_logs_init(&span_logs_item);
            result += span_logs_deserialize(item, &span_logs_item);
            vec_push(&obj->logs, span_logs_item);
        }
    } else {
        result += -1;
    }
    span_tags_init(&obj->tags);
    cJSON* cjson_obj_tags = cJSON_GetObjectItemCaseSensitive(json, "tags");
    if (cjson_obj_tags) {
        result += span_tags_deserialize(cjson_obj_tags, &obj->tags);
    } else {
        result += -1;
    }
    cJSON* cjson_obj_operationName = cJSON_GetObjectItemCaseSensitive(json, "operationName");
    if (cjson_obj_operationName) {
        obj->operationName = strdup(cJSON_GetStringValue(cjson_obj_operationName));
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_vec_warnings = cJSON_GetObjectItemCaseSensitive(json, "warnings");
    if (cjson_obj_vec_warnings) {
        cJSON* item;
        cJSON_ArrayForEach(item, cjson_obj_vec_warnings) {
            span_warnings span_warnings_item;
            span_warnings_init(&span_warnings_item);
            result += span_warnings_deserialize(item, &span_warnings_item);
            vec_push(&obj->warnings, span_warnings_item);
        }
    } else {
        result += -1;
    }
    cJSON* cjson_obj_parentSpanId = cJSON_GetObjectItemCaseSensitive(json, "parentSpanId");
    if (cjson_obj_parentSpanId) {
        obj->parentSpanId = strdup(cJSON_GetStringValue(cjson_obj_parentSpanId));
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_vec_references = cJSON_GetObjectItemCaseSensitive(json, "references");
    if (cjson_obj_vec_references) {
        cJSON* item;
        cJSON_ArrayForEach(item, cjson_obj_vec_references) {
            span_references span_references_item;
            span_references_init(&span_references_item);
            result += span_references_deserialize(item, &span_references_item);
            vec_push(&obj->references, span_references_item);
        }
    } else {
        result += -1;
    }
    cJSON* cjson_obj_startTime = cJSON_GetObjectItemCaseSensitive(json, "startTime");
    if (cjson_obj_startTime) {
        obj->startTime = strdup(cJSON_GetStringValue(cjson_obj_startTime));
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_duration = cJSON_GetObjectItemCaseSensitive(json, "duration");
    if (cjson_obj_duration) {
        obj->duration = cJSON_GetNumberValue(cjson_obj_duration);
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_spanId = cJSON_GetObjectItemCaseSensitive(json, "spanId");
    if (cjson_obj_spanId) {
        obj->spanId = strdup(cJSON_GetStringValue(cjson_obj_spanId));
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_vec_errors = cJSON_GetObjectItemCaseSensitive(json, "errors");
    if (cjson_obj_vec_errors) {
        cJSON* item;
        cJSON_ArrayForEach(item, cjson_obj_vec_errors) {
            span_errors span_errors_item;
            span_errors_init(&span_errors_item);
            result += span_errors_deserialize(item, &span_errors_item);
            vec_push(&obj->errors, span_errors_item);
        }
    } else {
        result += -1;
    }
    cJSON* cjson_obj_endTime = cJSON_GetObjectItemCaseSensitive(json, "endTime");
    if (cjson_obj_endTime) {
        obj->endTime = strdup(cJSON_GetStringValue(cjson_obj_endTime));
    }
    else {
        result += -1;
    }
    return result;

}


int span_deserialize_str(span* obj, char* json_str) {
    cJSON *json_parsed = cJSON_Parse(json_str);
    int result = span_deserialize(json_parsed, obj);
    cJSON_Delete(json_parsed);
    return result;
}

int span_deserialize_file(span* obj, char* file_path) {
    char* jsonStr;
    int result = -1;
    FILE *pFile = fopen(file_path, "r");
    if (pFile) {
        fseek(pFile, 0L, SEEK_END);
        long lenFile = ftell(pFile);
        fseek(pFile, 0L, SEEK_SET);
        jsonStr = malloc(sizeof(char) * (lenFile + 1));
        jsonStr[fread(jsonStr, 1, lenFile, pFile)] = '\0';
        result = span_deserialize_str(obj, jsonStr);
        free(jsonStr);
        fclose(pFile);
    }
    return result;
}

char* span_to_str(span* obj) {
    cJSON* cjson = cJSON_CreateObject();
    span_serialize(obj, cjson);
    char* tmp = cJSON_Print(cjson);
    cJSON_Delete(cjson);
    return tmp;
}

