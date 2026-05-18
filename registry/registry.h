#ifndef REGISTRY_H
#define REGISTRY_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "comm_fan_in_out.h"
#include "map.h"
#include "vec.h"
#include "service_info_json_tools.h"
#include "pthread.h"

typedef map_t(service_info) service_cluster_t;
typedef map_t(service_cluster_t) map_service_cluster_t;

typedef struct registry
{
    abs_comm* rs;
    map_service_cluster_t map_cluster;
    pthread_t pid;
    int thread_run_flag;
    pthread_mutex_t mutex_log;
}registry;

registry* registry_new(abs_comm_connect *rsct);
void registry_run(void* arg);
void registry_run_thread(registry* r);
void registry_stop(registry* r);
void registry_delete(registry* r);

char* map_service_cluster_serialize_to_str(map_service_cluster_t* map_cluster);




#ifdef __cplusplus
}
#endif
#endif // REGISTRY_H
