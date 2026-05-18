/*
 * 本文件由 json_to_c_struct.py 自动生成
 * base_msg_json_tools.c
 * 生成时间: 2024-11-26 14:40:53
 * 如无必要，请勿手动修改此文件
 */

#include "base_msg_json_tools.h"

void base_msg_init(base_msg* obj) {
    obj->msg = 0;
}


void base_msg_deinit(base_msg* obj) {
}


void base_msg_serialize(base_msg* obj, cJSON* json) {
    cJSON_AddItemToObject(json, "msg", cJSON_CreateNumber(obj->msg));
}


int base_msg_deserialize(cJSON* json, base_msg* obj) {
    int result = 0;
    cJSON* cjson_obj_msg = cJSON_GetObjectItemCaseSensitive(json, "msg");
    if (cjson_obj_msg) {
        obj->msg = cJSON_GetNumberValue(cjson_obj_msg);
    }
    else {
        result += -1;
    }
    return result;

}


int base_msg_deserialize_str(base_msg* obj, char* json_str) {
    cJSON *json_parsed = cJSON_Parse(json_str);
    int result = base_msg_deserialize(json_parsed, obj);
    cJSON_Delete(json_parsed);
    return result;
}

int base_msg_deserialize_file(base_msg* obj, char* file_path) {
    char* jsonStr;
    int result = -1;
    FILE *pFile = fopen(file_path, "r");
    if (pFile) {
        fseek(pFile, 0L, SEEK_END);
        long lenFile = ftell(pFile);
        fseek(pFile, 0L, SEEK_SET);
        jsonStr = malloc(sizeof(char) * (lenFile + 1));
        jsonStr[fread(jsonStr, 1, lenFile, pFile)] = '\0';
        result = base_msg_deserialize_str(obj, jsonStr);
        free(jsonStr);
        fclose(pFile);
    }
    return result;
}

char* base_msg_to_str(base_msg* obj) {
    cJSON* cjson = cJSON_CreateObject();
    base_msg_serialize(obj, cjson);
    char* tmp = cJSON_Print(cjson);
    cJSON_Delete(cjson);
    return tmp;
}

