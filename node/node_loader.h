#ifndef _NODE_LOADER_H
#define _NODE_LOADER_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "comm_fan_in_out.h"
#include "base_types.h"
#include "node_json_tools.h"
#include "map.h"
#include "pthread.h"
#include "rpc.h"

typedef struct node_loader
{
    abs_comm* rc;
    rpc_context* rpc_ctx;
    node n;
    pthread_t pid;
    int thread_run_flag;
    pthread_mutex_t mutex_log;
}node_loader;

node_loader* node_loader_new(char* file_path, abs_comm_connect* rct);
void node_loader_delete(node_loader* nc);
void node_loader_run(void* arg);
void node_loader_stop(node_loader* nc);
void node_loader_run_thread(node_loader* nc);


#ifdef __cplusplus
}
#endif
#endif
