#ifndef REGISTRY_SERVER_H
#define REGISTRY_SERVER_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "stdint.h"

//使用共享内存的场景
#if defined(__SHM_USE_ADDR__)
#define FILE_TRANS_START 0x10000000
#define FILE_TRANS_SIZE (16*1024*1024) // 16MB
#define AMP_SHM_START FILE_TRANS_START+FILE_TRANS_SIZE 
#endif

typedef int (*sub_topic_callback)(void* arg, void* msg, uint32_t len);

typedef struct abs_comm_connect
{
    char* ip;
    int port;
}abs_comm_connect;

typedef struct abs_comm
{
    void* client;
    char* client_name;
    abs_comm_connect rsc;
    int (*pub_msg)(struct abs_comm* arg, char* micreservice, void* msg, uint32_t len);
}abs_comm;

abs_comm* abs_comm_new(char* client_name, abs_comm_connect* rsc);

int abs_comm_sub(abs_comm * rc, char* topic_name, void *arg, sub_topic_callback recv_cb);

void abs_comm_delete(abs_comm* rs);

#ifdef __cplusplus
}
#endif
#endif // REGISTRY_SERVER_H
