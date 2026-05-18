/*
 * ���ļ��� json_to_c_struct.py �Զ�����
 * filetrans_json_tools.h
 * ����ʱ��: 2025-07-17 15:11:15
 * ���ޱ�Ҫ�������ֶ��޸Ĵ��ļ�
 */

#ifndef FILETRANS_JSON_TOOLS_H
#define FILETRANS_JSON_TOOLS_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "cJSON.h"
#include "vec.h"
#include "stdio.h"


typedef struct filetrans{
    int msg;
    char* target_node_id;
    char* filename;
    char* addr;
    int len;
} filetrans;


void filetrans_serialize(filetrans* obj, cJSON* json);

void filetrans_init(filetrans* obj);

void filetrans_deinit(filetrans* obj);

int filetrans_deserialize(cJSON* json, filetrans* obj);

int filetrans_deserialize_str(filetrans* obj, char* json_str);

int filetrans_deserialize_file(filetrans* obj, char* file_path);

char* filetrans_to_str(filetrans* obj);

#ifdef __cplusplus
}
#endif
#endif
