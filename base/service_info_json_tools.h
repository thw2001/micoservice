/*
 * 本文件由 json_to_c_struct.py 自动生成
 * service_info_json_tools.h
 * 生成时间: 2025-03-14 17:06:02
 * 如无必要，请勿手动修改此文件
 */

#ifndef SERVICE_INFO_JSON_TOOLS_H
#define SERVICE_INFO_JSON_TOOLS_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "cJSON.h"
#include "vec.h"
#include "stdio.h"

typedef struct service_info_dependencies{
    char* name;
} service_info_dependencies;


typedef vec_t(service_info_dependencies) vec_service_info_dependencies;

typedef struct service_info{
    char* status;
    char* endpoint;
    char* name;
    int timestamp;
    char* version;
    vec_service_info_dependencies dependencies;
    int msg;
    char* id;
    char* node_id;
} service_info;


void service_info_serialize(service_info* obj, cJSON* json);

int service_info_deserialize(cJSON* json, service_info* obj);

int service_info_deserialize_str(service_info* obj, char* json_str);

int service_info_deserialize_file(service_info* obj, char* file_path);

void service_info_init(service_info* obj);

void service_info_deinit(service_info* obj);

char* service_info_to_str(service_info* obj);

#ifdef __cplusplus
}
#endif
#endif
