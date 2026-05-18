/*
 * 本文件由 json_to_c_struct.py 自动生成
 * base_msg_json_tools.h
 * 生成时间: 2024-11-26 14:40:53
 * 如无必要，请勿手动修改此文件
 */

#ifndef BASE_MSG_JSON_TOOLS_H
#define BASE_MSG_JSON_TOOLS_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "cJSON.h"
#include "vec.h"
#include "stdio.h"


typedef struct base_msg{
    int msg;
} base_msg;


void base_msg_serialize(base_msg* obj, cJSON* json);

int base_msg_deserialize(cJSON* json, base_msg* obj);

int base_msg_deserialize_str(base_msg* obj, char* json_str);

int base_msg_deserialize_file(base_msg* obj, char* file_path);

void base_msg_init(base_msg* obj);

void base_msg_deinit(base_msg* obj);

char* base_msg_to_str(base_msg* obj);

#ifdef __cplusplus
}
#endif
#endif
