#include "registry.h"
#include "stdlib.h"
#include "stdio.h"
#include "base_types.h"
#include "base_msg_json_tools.h"
#include "cf_log.h"
#include "unistd.h"

/**
 * @brief 
 * 
 * @param r 
 * @param si 
 * @return int 
 *          1 : 没有把si存到map，需要释放
 *          0 : 把si存到map，不用释放
 */
int add_server_info_to_registey(registry* r, service_info si)
{
    int ret = -1;
    check_timeout(&si.timestamp, 0);
    service_cluster_t* sc = map_get(&r->map_cluster, si.name);
    if (NULL == sc)
    {
        log_debug("name:%s, id:%s, in node: %s new register", si.name, si.id, si.node_id);
        service_cluster_t new_sc;
        map_init(&new_sc);
        map_set(&new_sc, si.id, si);
        map_set(&r->map_cluster, si.name, new_sc);
        ret = 0;
    }
    else
    {
        service_info* tmp_si = map_get(sc, si.id);
        if (NULL == tmp_si)
        {
            log_debug("name:%s, id:%s in node: %s new add", si.name, si.id, si.node_id);
            map_set(sc, si.id, si);
            ret = 0;
        }
        else
        {
            log_trace("name:%s, id:%s in node: %s update timestamp", si.name, si.id, si.node_id);
            check_timeout(&tmp_si->timestamp, 0);
            ret = 1;
        }
        
    }

    return ret;
}

int registry_sub_topic_callback(void* arg, void* msg, uint32_t len)
{
    registry* r = arg;
    log_trace("len:%d, %s", len, (char*)msg);

    ((char*) msg)[len] = '\0';

    base_msg bs_msg;
    base_msg_init(&bs_msg);
    if (BASE_SUCCESS == base_msg_deserialize_str(&bs_msg, msg)){
        switch (bs_msg.msg)
        {
        case MSG_TYPE_SERVICE_INFO:
            ;
            service_info si;
            service_info_init(&si);
            if (BASE_SUCCESS == service_info_deserialize_str(&si, msg))
            {
                int need_free_si = add_server_info_to_registey(r, si);
                if (need_free_si)
                {
                    service_info_deinit(&si);
                }
            }else
            {
                log_error("service_info_deserialize_str failed! %s", (char*)msg);
            }
            

            break;
        
        default:
            log_error("unknow msg type:%d",bs_msg.msg);
            break;
        }
    }
    else
    {
        log_error("base_msg_deserialize_str failed! len: %d, msg: %s", len, (char*)msg);
    }
    
    base_msg_deinit(&bs_msg);
    return 0;
}

registry *registry_new(abs_comm_connect *rsct)
{
    registry* r = malloc(sizeof(registry));
    // abs_comm_connect rsct = {.ip = "127.0.0.1", .port = 19999};
    r->thread_run_flag = 0;
    r->rs = abs_comm_new("registry", rsct);

    if (NULL == r->rs)
    {
        free(r);
        return NULL;
    }

    int ret = abs_comm_sub(r->rs, REGISTRY_ID, r, registry_sub_topic_callback);

    if (BASE_SUCCESS != ret)
    {
        free(r);
        return NULL;
    }
    
    map_init(&r->map_cluster);
    
    pthread_mutex_init(&r->mutex_log, NULL);
    log_set_lock(log_mutex_lock, &r->mutex_log);

    return r;
}

void registry_delete(registry *r)
{
    abs_comm_delete(r->rs);
    const char *key;
    map_iter_t cluster_iter = map_iter(&r->map_cluster);
    while ((key = map_next(&r->map_cluster, &cluster_iter)))
    {
        service_cluster_t *cluster = map_get(&r->map_cluster, key);
        if (cluster)
        {
            map_iter_t si_iter = map_iter(cluster);
            while ((key = map_next(cluster, &si_iter)))
            {
                service_info* si =  map_get(cluster, key);
                if (si)
                {
                    log_debug("service deinit, name:%s, id:%s", si->name, si->id);
                    service_info_deinit(si);
                }
            }
            map_deinit(cluster);
        }
    }
    map_deinit(&r->map_cluster);
    free(r);
}

void send_cluster_to_target(registry* r, service_cluster_t* send_cluster, char* target_service)
{
    const char *key;
    if (send_cluster)
    {
        map_iter_t si_iter = map_iter(send_cluster);
        // 遍历集群
        while ((key = map_next(send_cluster, &si_iter)))
        {
            service_info *si = map_get(send_cluster, key);
            si->msg = MSG_TYPE_REGISTRY_TO_SERVICE;
            log_debug("find service: %s msg_type: %d", si->name, si->msg);
            char *tmp = service_info_to_str(si);
            r->rs->pub_msg(r->rs, target_service, tmp, strlen(tmp));
            free(tmp);
        }
    }
    
}

void push_dependency_to_service(registry* r)
{
    const char *key;
    map_iter_t cluster_iter = map_iter(&r->map_cluster);
    // 首先遍历每一个存活的cluster
    while ((key = map_next(&r->map_cluster, &cluster_iter)))
    {
        service_cluster_t *cluster = map_get(&r->map_cluster, key);
        if (cluster)
        {
            map_iter_t si_iter = map_iter(cluster);
            //  遍历每个cluster中的每个service实例
            while ((key = map_next(cluster, &si_iter)))
            {
                service_info *si = map_get(cluster, key);
                if (si)
                {
                    int i_send_si_de = 0;
                    service_info_dependencies* iter_send_si_de;
                    // 根据实例所依赖的service，向这个实例发送service的集群的信息
                    vec_foreach_ptr(&si->dependencies, iter_send_si_de, i_send_si_de){
                        service_cluster_t* send_cluster = map_get(&r->map_cluster, iter_send_si_de->name);
                        send_cluster_to_target(r, send_cluster, si->id);
                    }
                }
            }
        }
    }
}

void check_service_instance_timeout(registry* r)
{
    const char *key;
    map_iter_t cluster_iter = map_iter(&r->map_cluster);
    while ((key = map_next(&r->map_cluster, &cluster_iter)))
    {
        service_cluster_t *cluster = map_get(&r->map_cluster, key);
        if (cluster)
        {
            map_iter_t si_iter = map_iter(cluster);
            while ((key = map_next(cluster, &si_iter)))
            {
                service_info* si =  map_get(cluster, key);
                if (si)
                {
                    if (check_timeout(&si->timestamp, 5))
                    {
                        log_debug("timeout  name:%s, id:%s", si->name, si->id);
                        service_info_deinit(si);
                        map_remove(cluster, key);
                        si = NULL;
                    }
                }
            }
        }
    }
}

void registry_run(void* arg)
{
    registry* r = arg;
    r->thread_run_flag = THREAD_RUNING;
    while (THREAD_RUNING == r->thread_run_flag)
    {
        check_service_instance_timeout(r);
        push_dependency_to_service(r);
        sleep(2);
    }
}

void registry_stop(registry* r)
{
    r->thread_run_flag = THREAD_STOPING;
    pthread_join(r->pid, NULL);
}

void registry_run_thread(registry* r)
{
    pthread_create(&r->pid, NULL, (void *)registry_run, (void*)r);
}

char* map_service_cluster_serialize_to_str(map_service_cluster_t* map_cluster)
{
    if (!map_cluster) return NULL;

    cJSON* root = cJSON_CreateObject();
    if (!root) return NULL;

    const char* cluster_key;
    map_iter_t cluster_iter = map_iter(map_cluster);
    while ((cluster_key = map_next(map_cluster, &cluster_iter)))
    {
        service_cluster_t* cluster = map_get(map_cluster, cluster_key);
        if (cluster)
        {
            cJSON* cluster_obj = cJSON_CreateObject();
            const char* service_key;
            map_iter_t service_iter = map_iter(cluster);
            while ((service_key = map_next(cluster, &service_iter)))
            {
                service_info* si = map_get(cluster, service_key);
                if (si)
                {
                    cJSON* si_json = cJSON_CreateObject();
                    if (si_json)
                    {
                        service_info_serialize(si, si_json);
                        cJSON_AddItemToObject(cluster_obj, service_key, si_json);
                    }
                }
            }
            cJSON_AddItemToObject(root, cluster_key, cluster_obj);
        }
    }

    char* result = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return result;
}
