#ifndef MICO_SERVICE_H
#define MICO_SERVICE_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "comm_fan_in_out.h"
#include "service_info_json_tools.h"
#include "node_json_tools.h"
#include "pthread.h"
#include "map.h"

#define METHOD_RANDOM 0
#define METHOD_ONE  1

typedef struct service_local_info
{
    service_info si;
}service_local_info;

typedef map_t(service_local_info) map_service_cache_t;

typedef struct service_local_cache
{
    map_service_cache_t msc;
    int total;  // 总实例数
    int count;  // 当前计数
}service_local_cache;

typedef map_t(service_local_cache) map_dependency_cache_t;

typedef struct mico_service mico_service;
typedef struct mico_service
{
    abs_comm* rc;
    abs_comm* usr_rce;
    service_info* info;
    node nd;
    pthread_t pid;
    int thread_run_flag;
    map_dependency_cache_t dependency_cache;
    pthread_mutex_t mutex_log;
}mico_service;

mico_service *mico_service_new(char* service_name, abs_comm_connect* rct);
mico_service *mico_service_info_new(char* service_info_file, abs_comm_connect* rct);
mico_service *mico_service_info_str_new(char* service_info_str, abs_comm_connect* rct);
int mico_service_node(mico_service * ms, char* node_json_file);
void mico_service_set_status(mico_service * ms, char* status);
void mico_service_set_version(mico_service * ms, char* version);
void mico_service_set_endpoint(mico_service * ms, char* endpoint);
void mico_service_run(void *arg);
void mico_service_run_thread(mico_service *ms);
void mico_service_stop(mico_service *ms);
void mico_service_delete(mico_service* ms);
int mico_service_add_dependency(mico_service * ms, service_info_dependencies* dependency);
service_info* mico_service_get_instance(mico_service *ms, char* service_name, int method);

int mico_service_sub(mico_service* ms, char* topic, void *arg, sub_topic_callback recv_cb);
int mico_service_pub(mico_service* ms, char* topic, void* msg, uint32_t len);

#ifdef __cplusplus
}
#endif
#endif // MICO_SERVICE_H
