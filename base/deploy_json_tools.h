/*
 * 本文件由 json_to_c_struct.py 自动生成
 * deploy_json_tools.h
 * 生成时间: 2025-03-17 14:07:32
 * 如无必要，请勿手动修改此文件
 */

#ifndef DEPLOY_JSON_TOOLS_H
#define DEPLOY_JSON_TOOLS_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "cJSON.h"
#include "vec.h"
#include "stdio.h"

typedef struct deploy_config{
    int msg;
    char* node_id;
    char* command;
} deploy_config;


typedef vec_t(deploy_config) vec_deploy_config;

typedef struct deploy{
    int msg;
    vec_deploy_config config;
} deploy;


void deploy_serialize(deploy* obj, cJSON* json);

void deploy_config_serialize(deploy_config* obj, cJSON* json);

void deploy_init(deploy* obj);

void deploy_config_init(deploy_config* obj);

void deploy_deinit(deploy* obj);

void deploy_config_deinit(deploy_config* obj);

int deploy_deserialize(cJSON* json, deploy* obj);

int deploy_config_deserialize(cJSON* json, deploy_config* obj);

int deploy_deserialize_str(deploy* obj, char* json_str);

int deploy_deserialize_file(deploy* obj, char* file_path);

char* deploy_to_str(deploy* obj);

#ifdef __cplusplus
}
#endif
#endif
