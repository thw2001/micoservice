/*
 * ���ļ��� json_to_c_struct.py �Զ�����
 * filetrans_json_tools.c
 * ����ʱ��: 2025-07-17 15:11:15
 * ���ޱ�Ҫ�������ֶ��޸Ĵ��ļ�
 */

#include "filetrans_json_tools.h"

void filetrans_init(filetrans* obj) {
    obj->msg = 0;
    obj->target_node_id = NULL;
    obj->filename = NULL;
    obj->addr = NULL;
    obj->len = 0;
}


void filetrans_deinit(filetrans* obj) {
    if (NULL != obj->target_node_id)
        free(obj->target_node_id);
    if (NULL != obj->filename)
        free(obj->filename);
    if (NULL != obj->addr)
        free(obj->addr);
}


void filetrans_serialize(filetrans* obj, cJSON* json) {
    cJSON_AddItemToObject(json, "msg", cJSON_CreateNumber(obj->msg));
    cJSON_AddItemToObject(json, "target_node_id", cJSON_CreateString(obj->target_node_id));
    cJSON_AddItemToObject(json, "filename", cJSON_CreateString(obj->filename));
    cJSON_AddItemToObject(json, "addr", cJSON_CreateString(obj->addr));
    cJSON_AddItemToObject(json, "len", cJSON_CreateNumber(obj->len));
}


int filetrans_deserialize(cJSON* json, filetrans* obj) {
    int result = 0;
    cJSON* cjson_obj_msg = cJSON_GetObjectItemCaseSensitive(json, "msg");
    if (cjson_obj_msg) {
        obj->msg = cJSON_GetNumberValue(cjson_obj_msg);
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_target_node_id = cJSON_GetObjectItemCaseSensitive(json, "target_node_id");
    if (cjson_obj_target_node_id) {
        obj->target_node_id = strdup(cJSON_GetStringValue(cjson_obj_target_node_id));
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_filename = cJSON_GetObjectItemCaseSensitive(json, "filename");
    if (cjson_obj_filename) {
        obj->filename = strdup(cJSON_GetStringValue(cjson_obj_filename));
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_addr = cJSON_GetObjectItemCaseSensitive(json, "addr");
    if (cjson_obj_addr) {
        obj->addr = strdup(cJSON_GetStringValue(cjson_obj_addr));
    }
    else {
        result += -1;
    }
    cJSON* cjson_obj_len = cJSON_GetObjectItemCaseSensitive(json, "len");
    if (cjson_obj_len) {
        obj->len = cJSON_GetNumberValue(cjson_obj_len);
    }
    else {
        result += -1;
    }
    return result;

}


int filetrans_deserialize_str(filetrans* obj, char* json_str) {
    cJSON *json_parsed = cJSON_Parse(json_str);
    int result = filetrans_deserialize(json_parsed, obj);
    cJSON_Delete(json_parsed);
    return result;
}

int filetrans_deserialize_file(filetrans* obj, char* file_path) {
    char* jsonStr;
    int result = -1;
    FILE *pFile = fopen(file_path, "r");
    if (pFile) {
        fseek(pFile, 0L, SEEK_END);
        long lenFile = ftell(pFile);
        fseek(pFile, 0L, SEEK_SET);
        jsonStr = malloc(sizeof(char) * (lenFile + 1));
        jsonStr[fread(jsonStr, 1, lenFile, pFile)] = '\0';
        result = filetrans_deserialize_str(obj, jsonStr);
        free(jsonStr);
        fclose(pFile);
    }
    return result;
}

char* filetrans_to_str(filetrans* obj) {
    cJSON* cjson = cJSON_CreateObject();
    filetrans_serialize(obj, cjson);
    char* tmp = cJSON_PrintUnformatted(cjson);
    cJSON_Delete(cjson);
    return tmp;
}

