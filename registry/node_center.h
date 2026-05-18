#ifndef _NODE_CENTER_H
#define _NODE_CENTER_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "comm_fan_in_out.h"
#include "map.h"
#include "pthread.h"
#include "abs_comm.h"
#include "rpc.h"
#include "base_types.h"
#include "node_json_tools.h"
#include "deploy_json_tools.h"
#include "filetrans_json_tools.h"

typedef map_t(node) node_cluster_t;
typedef map_t(node_cluster_t) map_node_cluster_t;

typedef struct node_center
{
    abs_comm* rs;
    rpc_context* rpc_ctx;
    map_node_cluster_t map_cluster;
    pthread_t pid;
    int thread_run_flag;
    pthread_mutex_t file_mutex;
}node_center;

node_center* node_center_new(abs_comm_connect* rsct);
void node_center_delete(node_center* nc);
void node_center_run(void* arg);
void node_center_stop(node_center* nc);
void node_center_run_thread(node_center* nc);

int node_center_filetrans_cmd(node_center* nc,char* filename,char* target_node_id);
int node_center_deploy_cmd(node_center* nc, char* node_name, char* exec_cmd);
char* map_node_cluster_serialize_to_str(map_node_cluster_t* map_cluster);


#define FILLTRANS_ERR_OPEN -1
#define FILLTRANS_ERR_FSTAT -2
#define FILLTRANS_ERR_OVERSIZE -3
#define FILLTRANS_ERR_READ -4

#ifdef __cplusplus
}
#endif
#endif
