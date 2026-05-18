/*
 * 本文件由 json_to_c_struct.py 自动生成
 * deploy_json_tools.c
 * 生成时间: 2025-03-17 14:07:32
 * 如无必要，请勿手动修改此文件
 */

#include "deploy_json_tools.h"

void deploy_config_init(deploy_config* obj) {
    obj->msg = 0;
    obj->node_id = NULL;
    obj->command = NULL;
}

void deploy_init(deploy* obj) {
    obj->msg = 0;
    vec_init(&obj->config);
}


void deploy_config_deinit(deploy_config* obj) {
    if (NULL != obj->node_id)
        free(obj->node_id);
    if (NULL != obj->command)
        free(obj->command);
}

void deploy_deinit(deploy* obj) {
    int deploy_config_i = 0;
    deploy_config* deploy_config_iter;
    vec_foreach_ptr(&obj->config, deploy_config_iter, deploy_config_i) {
        deploy_config_deinit(deploy_config_iter);
    }
    vec_deinit(&obj->config);
}


void deploy_config_serialize(deploy_config* obj, cJSON* json) {
    cJSON_AddItemToObject(json, "msg", cJSON_CreateNumber(obj->msg));
    cJSON_AddItemToObject(json, "node_id", cJSON_CreateString(obj->node_id));
    cJSON_AddItemToObject(json, "command", cJSON_CreateString(obj->command));
}

void deploy_serialize(deploy* obj, cJSON* json) {
    cJSON_AddItemToObject(json, "msg", cJSON_CreateNumber(obj->msg));
    cJSON* cjson_obj_vec_config = cJSON_CreateArray();
    int deploy_config_i = 0;
    deploy_config* deploy_config_iter;
    vec_foreach_ptr(&obj->config, deploy_config_iter, deploy_config_i) {
        cJSON* tmp_obj = cJSON_CreateObject();
        deploy_config_serialize(deploy_config_iter, tmp_obj);
        cJSON_AddItemToArray(cjson_obj_vec_config, tmp_obj);
    }
    cJSON_AddItemToObject(json, "config", cjson_obj_vec_config);
}


int deploy_config_deserialize(cJSON* json, deploy_config* obj) {
    int result = 0;
    cJSON* cjson_obj_msg = cJSON_GetObjectItemCaseSensitive(json, "msg");
    if (cjson_obj_msg) {
        obj->msg = cJSON_GetNumberValue(cjson_obj_msg);
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
    cJSON* cjson_obj_command = cJSON_GetObjectItemCaseSensitive(json, "command");
    if (cjson_obj_command) {
        obj->command = strdup(cJSON_GetStringValue(cjson_obj_command));
    }
    else {
        result += -1;
    }
    return result;

}

int deploy_deserialize(cJSON* json, deploy* obj) {
    int result = 0;
    cJSON* cjson_obj_msg = cJSON_GetObjectItemCaseSensitive(json, "msg");
    if (cjson_obj_msg) {
        obj->msg = cJSON_GetNumberValue(cjson_obj_msg);
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_vec_config = cJSON_GetObjectItemCaseSensitive(json, "config");
    if (cjson_obj_vec_config) {
        cJSON* item;
        cJSON_ArrayForEach(item, cjson_obj_vec_config) {
            deploy_config deploy_config_item;
            deploy_config_init(&deploy_config_item);
            result += deploy_config_deserialize(item, &deploy_config_item);
            vec_push(&obj->config, deploy_config_item);
        }
    } else {
        result += -1;
    }
    return result;

}


int deploy_deserialize_str(deploy* obj, char* json_str) {
    cJSON *json_parsed = cJSON_Parse(json_str);
    int result = deploy_deserialize(json_parsed, obj);
    cJSON_Delete(json_parsed);
    return result;
}

int deploy_deserialize_file(deploy* obj, char* file_path) {
    char* jsonStr;
    int result = -1;
    FILE *pFile = fopen(file_path, "r");
    if (pFile) {
        fseek(pFile, 0L, SEEK_END);
        long lenFile = ftell(pFile);
        fseek(pFile, 0L, SEEK_SET);
        jsonStr = malloc(sizeof(char) * (lenFile + 1));
        jsonStr[fread(jsonStr, 1, lenFile, pFile)] = '\0';
        result = deploy_deserialize_str(obj, jsonStr);
        free(jsonStr);
        fclose(pFile);
    }
    return result;
}

char* deploy_to_str(deploy* obj) {
    cJSON* cjson = cJSON_CreateObject();
    deploy_serialize(obj, cjson);
    char* tmp = cJSON_PrintUnformatted(cjson);
    cJSON_Delete(cjson);
    return tmp;
}

