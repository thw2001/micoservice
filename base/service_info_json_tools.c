/*
 * 本文件由 json_to_c_struct.py 自动生成
 * service_info_json_tools.c
 * 生成时间: 2025-03-14 17:06:02
 * 如无必要，请勿手动修改此文件
 */

#include "service_info_json_tools.h"

void service_info_dependencies_init(service_info_dependencies* obj) {
    obj->name = NULL;
}

void service_info_init(service_info* obj) {
    obj->status = NULL;
    obj->endpoint = NULL;
    obj->name = NULL;
    obj->timestamp = 0;
    obj->version = NULL;
    vec_init(&obj->dependencies);
    obj->msg = 0;
    obj->id = NULL;
    obj->node_id = NULL;
}


void service_info_dependencies_deinit(service_info_dependencies* obj) {
    if (NULL != obj->name)
        free(obj->name);
}

void service_info_deinit(service_info* obj) {
    if (NULL != obj->status)
        free(obj->status);
    if (NULL != obj->endpoint)
        free(obj->endpoint);
    if (NULL != obj->name)
        free(obj->name);
    if (NULL != obj->version)
        free(obj->version);
    int service_info_dependencies_i = 0;
    service_info_dependencies* service_info_dependencies_iter;
    vec_foreach_ptr(&obj->dependencies, service_info_dependencies_iter, service_info_dependencies_i) {
        service_info_dependencies_deinit(service_info_dependencies_iter);
    }
    vec_deinit(&obj->dependencies);
    if (NULL != obj->id)
        free(obj->id);
    if (NULL != obj->node_id)
        free(obj->node_id);
}


void service_info_dependencies_serialize(service_info_dependencies* obj, cJSON* json) {
    cJSON_AddItemToObject(json, "name", cJSON_CreateString(obj->name));
}

void service_info_serialize(service_info* obj, cJSON* json) {
    cJSON_AddItemToObject(json, "status", cJSON_CreateString(obj->status));
    cJSON_AddItemToObject(json, "endpoint", cJSON_CreateString(obj->endpoint));
    cJSON_AddItemToObject(json, "name", cJSON_CreateString(obj->name));
    cJSON_AddItemToObject(json, "timestamp", cJSON_CreateNumber(obj->timestamp));
    cJSON_AddItemToObject(json, "version", cJSON_CreateString(obj->version));
    cJSON* cjson_obj_vec_dependencies = cJSON_CreateArray();
    int service_info_dependencies_i = 0;
    service_info_dependencies* service_info_dependencies_iter;
    vec_foreach_ptr(&obj->dependencies, service_info_dependencies_iter, service_info_dependencies_i) {
        cJSON* tmp_obj = cJSON_CreateObject();
        service_info_dependencies_serialize(service_info_dependencies_iter, tmp_obj);
        cJSON_AddItemToArray(cjson_obj_vec_dependencies, tmp_obj);
    }
    cJSON_AddItemToObject(json, "dependencies", cjson_obj_vec_dependencies);
    cJSON_AddItemToObject(json, "msg", cJSON_CreateNumber(obj->msg));
    cJSON_AddItemToObject(json, "id", cJSON_CreateString(obj->id));
    cJSON_AddItemToObject(json, "node_id", cJSON_CreateString(obj->node_id));
}


int service_info_dependencies_deserialize(cJSON* json, service_info_dependencies* obj) {
    int result = 0;
    cJSON* cjson_obj_name = cJSON_GetObjectItemCaseSensitive(json, "name");
    if (cjson_obj_name) {
        obj->name = strdup(cJSON_GetStringValue(cjson_obj_name));
    }
    else {
        result += -1;
    }
    return result;

}

int service_info_deserialize(cJSON* json, service_info* obj) {
    int result = 0;
    cJSON* cjson_obj_status = cJSON_GetObjectItemCaseSensitive(json, "status");
    if (cjson_obj_status) {
        obj->status = strdup(cJSON_GetStringValue(cjson_obj_status));
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_endpoint = cJSON_GetObjectItemCaseSensitive(json, "endpoint");
    if (cjson_obj_endpoint) {
        obj->endpoint = strdup(cJSON_GetStringValue(cjson_obj_endpoint));
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_name = cJSON_GetObjectItemCaseSensitive(json, "name");
    if (cjson_obj_name) {
        obj->name = strdup(cJSON_GetStringValue(cjson_obj_name));
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_timestamp = cJSON_GetObjectItemCaseSensitive(json, "timestamp");
    if (cjson_obj_timestamp) {
        obj->timestamp = cJSON_GetNumberValue(cjson_obj_timestamp);
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_version = cJSON_GetObjectItemCaseSensitive(json, "version");
    if (cjson_obj_version) {
        obj->version = strdup(cJSON_GetStringValue(cjson_obj_version));
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_vec_dependencies = cJSON_GetObjectItemCaseSensitive(json, "dependencies");
    if (cjson_obj_vec_dependencies) {
        cJSON* item;
        cJSON_ArrayForEach(item, cjson_obj_vec_dependencies) {
            service_info_dependencies service_info_dependencies_item;
            service_info_dependencies_init(&service_info_dependencies_item);
            result += service_info_dependencies_deserialize(item, &service_info_dependencies_item);
            vec_push(&obj->dependencies, service_info_dependencies_item);
        }
    } else {
        result += -1;
    }
    cJSON* cjson_obj_msg = cJSON_GetObjectItemCaseSensitive(json, "msg");
    if (cjson_obj_msg) {
        obj->msg = cJSON_GetNumberValue(cjson_obj_msg);
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_id = cJSON_GetObjectItemCaseSensitive(json, "id");
    if (cjson_obj_id) {
        obj->id = strdup(cJSON_GetStringValue(cjson_obj_id));
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_node_id = cJSON_GetObjectItemCaseSensitive(json, "node_id");
    if (cjson_obj_node_id) {
        obj->node_id = strdup(cJSON_GetStringValue(cjson_obj_node_id));
    }
    else {
        result += -1;
    }
    return result;

}


int service_info_deserialize_str(service_info* obj, char* json_str) {
    cJSON *json_parsed = cJSON_Parse(json_str);
    int result = service_info_deserialize(json_parsed, obj);
    cJSON_Delete(json_parsed);
    return result;
}

int service_info_deserialize_file(service_info* obj, char* file_path) {
    char* jsonStr;
    int result = -1;
    FILE *pFile = fopen(file_path, "r");
    if (pFile) {
        fseek(pFile, 0L, SEEK_END);
        long lenFile = ftell(pFile);
        fseek(pFile, 0L, SEEK_SET);
        jsonStr = malloc(sizeof(char) * (lenFile + 1));
        jsonStr[fread(jsonStr, 1, lenFile, pFile)] = '\0';
        result = service_info_deserialize_str(obj, jsonStr);
        free(jsonStr);
        fclose(pFile);
    }
    return result;
}

char* service_info_to_str(service_info* obj) {
    cJSON* cjson = cJSON_CreateObject();
    service_info_serialize(obj, cjson);
    char* tmp = cJSON_Print(cjson);
    cJSON_Delete(cjson);
    return tmp;
}

