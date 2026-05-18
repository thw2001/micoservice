/*
 * 本文件由 json_to_c_struct.py 自动生成
 * node_json_tools.c
 * 生成时间: 2025-07-04 15:02:47
 * 如无必要，请勿手动修改此文件
 */

#include "node_json_tools.h"

void node_device_init(node_device* obj) {
    obj->name = NULL;
}

void node_init(node* obj) {
    obj->msg = 0;
    obj->name = NULL;
    obj->id = NULL;
    obj->os = NULL;
    obj->arch = NULL;
    obj->type = NULL;
    obj->suffix = NULL;
    obj->timestamp = 0;
    vec_init(&obj->device);
}


void node_device_deinit(node_device* obj) {
    if (NULL != obj->name)
        free(obj->name);
}

void node_deinit(node* obj) {
    if (NULL != obj->name)
        free(obj->name);
    if (NULL != obj->id)
        free(obj->id);
    if (NULL != obj->os)
        free(obj->os);
    if (NULL != obj->arch)
        free(obj->arch);
    if (NULL != obj->type)
        free(obj->type);
    if (NULL != obj->suffix)
        free(obj->suffix);
    int node_device_i = 0;
    node_device* node_device_iter;
    vec_foreach_ptr(&obj->device, node_device_iter, node_device_i) {
        node_device_deinit(node_device_iter);
    }
    vec_deinit(&obj->device);
}


void node_device_serialize(node_device* obj, cJSON* json) {
    cJSON_AddItemToObject(json, "name", cJSON_CreateString(obj->name));
}

void node_serialize(node* obj, cJSON* json) {
    cJSON_AddItemToObject(json, "msg", cJSON_CreateNumber(obj->msg));
    cJSON_AddItemToObject(json, "name", cJSON_CreateString(obj->name));
    cJSON_AddItemToObject(json, "id", cJSON_CreateString(obj->id));
    cJSON_AddItemToObject(json, "os", cJSON_CreateString(obj->os));
    cJSON_AddItemToObject(json, "arch", cJSON_CreateString(obj->arch));
    cJSON_AddItemToObject(json, "type", cJSON_CreateString(obj->type));
    cJSON_AddItemToObject(json, "suffix", cJSON_CreateString(obj->suffix));
    cJSON_AddItemToObject(json, "timestamp", cJSON_CreateNumber(obj->timestamp));
    cJSON* cjson_obj_vec_device = cJSON_CreateArray();
    int node_device_i = 0;
    node_device* node_device_iter;
    vec_foreach_ptr(&obj->device, node_device_iter, node_device_i) {
        cJSON* tmp_obj = cJSON_CreateObject();
        node_device_serialize(node_device_iter, tmp_obj);
        cJSON_AddItemToArray(cjson_obj_vec_device, tmp_obj);
    }
    cJSON_AddItemToObject(json, "device", cjson_obj_vec_device);
}


int node_device_deserialize(cJSON* json, node_device* obj) {
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

int node_deserialize(cJSON* json, node* obj) {
    int result = 0;
    cJSON* cjson_obj_msg = cJSON_GetObjectItemCaseSensitive(json, "msg");
    if (cjson_obj_msg) {
        obj->msg = cJSON_GetNumberValue(cjson_obj_msg);
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
    cJSON* cjson_obj_id = cJSON_GetObjectItemCaseSensitive(json, "id");
    if (cjson_obj_id) {
        obj->id = strdup(cJSON_GetStringValue(cjson_obj_id));
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_os = cJSON_GetObjectItemCaseSensitive(json, "os");
    if (cjson_obj_os) {
        obj->os = strdup(cJSON_GetStringValue(cjson_obj_os));
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_arch = cJSON_GetObjectItemCaseSensitive(json, "arch");
    if (cjson_obj_arch) {
        obj->arch = strdup(cJSON_GetStringValue(cjson_obj_arch));
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_type = cJSON_GetObjectItemCaseSensitive(json, "type");
    if (cjson_obj_type) {
        obj->type = strdup(cJSON_GetStringValue(cjson_obj_type));
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_suffix = cJSON_GetObjectItemCaseSensitive(json, "suffix");
    if (cjson_obj_suffix) {
        obj->suffix = strdup(cJSON_GetStringValue(cjson_obj_suffix));
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
    cJSON* cjson_obj_vec_device = cJSON_GetObjectItemCaseSensitive(json, "device");
    if (cjson_obj_vec_device) {
        cJSON* item;
        cJSON_ArrayForEach(item, cjson_obj_vec_device) {
            node_device node_device_item;
            node_device_init(&node_device_item);
            result += node_device_deserialize(item, &node_device_item);
            vec_push(&obj->device, node_device_item);
        }
    } else {
        result += -1;
    }
    return result;

}


int node_deserialize_str(node* obj, char* json_str) {
    cJSON *json_parsed = cJSON_Parse(json_str);
    int result = node_deserialize(json_parsed, obj);
    cJSON_Delete(json_parsed);
    return result;
}

int node_deserialize_file(node* obj, char* file_path) {
    char* jsonStr;
    int result = -1;
    FILE *pFile = fopen(file_path, "r");
    if (pFile) {
        fseek(pFile, 0L, SEEK_END);
        long lenFile = ftell(pFile);
        fseek(pFile, 0L, SEEK_SET);
        jsonStr = malloc(sizeof(char) * (lenFile + 1));
        jsonStr[fread(jsonStr, 1, lenFile, pFile)] = '\0';
        result = node_deserialize_str(obj, jsonStr);
        free(jsonStr);
        fclose(pFile);
    }
    return result;
}

char* node_to_str(node* obj) {
    cJSON* cjson = cJSON_CreateObject();
    node_serialize(obj, cjson);
    char* tmp = cJSON_PrintUnformatted(cjson);
    cJSON_Delete(cjson);
    return tmp;
}

