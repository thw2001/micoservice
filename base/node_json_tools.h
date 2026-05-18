/*
 * 本文件由 json_to_c_struct.py 自动生成
 * node_json_tools.h
 * 生成时间: 2025-07-04 15:02:47
 * 如无必要，请勿手动修改此文件
 */

#ifndef NODE_JSON_TOOLS_H
#define NODE_JSON_TOOLS_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "cJSON.h"
#include "vec.h"
#include "stdio.h"

typedef struct node_device{
    char* name;
} node_device;


typedef vec_t(node_device) vec_node_device;

typedef struct node{
    int msg;
    char* name;
    char* id;
    char* os;
    char* arch;
    char* type;
    char* suffix;
    int timestamp;
    vec_node_device device;
} node;


void node_serialize(node* obj, cJSON* json);

void node_device_serialize(node_device* obj, cJSON* json);

void node_init(node* obj);

void node_device_init(node_device* obj);

void node_deinit(node* obj);

void node_device_deinit(node_device* obj);

int node_deserialize(cJSON* json, node* obj);

int node_device_deserialize(cJSON* json, node_device* obj);

int node_deserialize_str(node* obj, char* json_str);

int node_deserialize_file(node* obj, char* file_path);

char* node_to_str(node* obj);

#ifdef __cplusplus
}
#endif
#endif
