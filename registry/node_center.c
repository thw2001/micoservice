// #include <fcntl.h>
// #include <sys/stat.h>
#include "node_center.h"
#include "stdlib.h"
#include "stdio.h"
#include "cf_log.h"
// #include "unistd.h"
// #include "rpc.h"
// #include "cJSON.h"
// #include "base_types.h"
#include "base_msg_json_tools.h"

//cluster 相关:生成集群名称
char* generate_cluster_name(node* obj)
{
    int len = strlen(obj->type) + strlen(obj->os) + strlen(obj->arch) + 3;
    char* cluster = malloc(len);
    snprintf(cluster, len, "%s:%s:%s", 
             (obj->type),
             (obj->os),
             (obj->arch));
    return cluster;
}
//cluster 相关:添加节点到集群
static int add_node_to_cluster(node_center* nc, node n)
{
    int ret = -1;
    check_timeout(&n.timestamp, 0);
    
    char* cluster_name = generate_cluster_name(&n);
    node_cluster_t* cluster = map_get(&nc->map_cluster, cluster_name);
    if (NULL == cluster) {
        log_debug("node cluster:%s new register", cluster_name);
        node_cluster_t new_cluster;
        map_init(&new_cluster);
        map_set(&new_cluster, n.id, n);
        map_set(&nc->map_cluster, cluster_name, new_cluster);
        ret = 0;
    } else {
        node* existing = map_get(cluster, n.id);
        if (NULL == existing) {
            log_debug("node:%s new add to cluster:%s", n.id, cluster_name);
            map_set(cluster, n.id, n);
            ret = 0;
        } else {
            log_trace("node:%s update timestamp", n.id);
            check_timeout(&existing->timestamp, 0);
            ret = 1;
        }
    }
    free(cluster_name);
    return ret;
}
//cluster 相关:序列化工具
char* map_node_cluster_serialize_to_str(map_node_cluster_t* map_cluster)
{
    if (!map_cluster) return NULL;

    cJSON* root = cJSON_CreateObject();
    if (!root) return NULL;

    const char* cluster_key;
    map_iter_t cluster_iter = map_iter(map_cluster);
    while ((cluster_key = map_next(map_cluster, &cluster_iter)))
    {
        node_cluster_t* cluster = map_get(map_cluster, cluster_key);
        if (cluster)
        {
            cJSON* cluster_obj = cJSON_CreateObject();
            const char* node_key;
            map_iter_t node_iter = map_iter(cluster);
            while ((node_key = map_next(cluster, &node_iter)))
            {
                node* n = map_get(cluster, node_key);
                if (n)
                {
                    cJSON* node_json = cJSON_CreateObject();
                    if (node_json)
                    {
                        node_serialize(n, node_json);
                        cJSON_AddItemToObject(cluster_obj, node_key, node_json);
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

//命令格式的通信，目前只用于心跳和日志消息
static int node_center_recv_msg(void* arg, void* msg, uint32_t len)
{
    node_center* nc = arg;
    log_trace("node center recv len:%d, %s", len, (char*)msg);

    ((char*) msg)[len] = '\0';
    
    base_msg bs_msg;
    base_msg_init(&bs_msg);
    if (BASE_SUCCESS == base_msg_deserialize_str(&bs_msg, msg)) {
        switch (bs_msg.msg) {
        case MSG_TYPE_NODE_INFP: {
            node n;
            node_init(&n);
            if (BASE_SUCCESS == node_deserialize_str(&n, msg)) {
                int need_free = add_node_to_cluster(nc, n);
                if (need_free) {
                    node_deinit(&n);
                }
            }
            break;
        }
        default:
            log_warn("unsupported msg type:%d", bs_msg.msg);
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
//检查节点心跳是否超时
static void check_node_timeout(node_center* nc)
{
    const char* key;
    map_iter_t cluster_iter = map_iter(&nc->map_cluster);
    while ((key = map_next(&nc->map_cluster, &cluster_iter))) {
        node_cluster_t* cluster = map_get(&nc->map_cluster, key);
        if (cluster) {
            map_iter_t node_iter = map_iter(cluster);
            while ((key = map_next(cluster, &node_iter))) {
                node* n = map_get(cluster, key);
                if (n && check_timeout(&n->timestamp, 5)) {
                    log_debug("node timeout: %s", n->id);
                    node_deinit(n);
                    map_remove(cluster, key);
                }
            }
        }
    }
}

//创建node_center
node_center* node_center_new(abs_comm_connect* rsct)
{
    node_center* nc = malloc(sizeof(node_center));
    nc->rs = abs_comm_new("node_center", rsct);//初始化中间件
    if (NULL == nc->rs) {
        free(nc);
        return NULL;
    }
    int ret = abs_comm_sub(nc->rs, NODE_CENTER, nc, node_center_recv_msg);//订阅消息
    if (BASE_SUCCESS != ret)
    {
        free(nc);
        return NULL;
    }
    nc->rpc_ctx = rpc_context_new(nc->rs);
    map_init(&nc->map_cluster);
    nc->pid = 0;
    nc->thread_run_flag = 0;
    pthread_mutex_init(&(nc->file_mutex), NULL);
    return nc;
}
//删除node_center,必须要先停止node_center线程再回收资源
void node_center_delete(node_center* nc)
{
    if (nc->thread_run_flag != THREAD_STOPING){
        node_center_stop(nc);//确保线程停止
    }
    abs_comm_delete(nc->rs);
    rpc_context_delete(nc->rpc_ctx);
    const char* key;//释放map_cluster中的每个node
    map_iter_t cluster_iter = map_iter(&nc->map_cluster);
    while ((key = map_next(&nc->map_cluster, &cluster_iter))) {
        node_cluster_t* cluster = map_get(&nc->map_cluster, key);
        if (cluster) {
            map_iter_t node_iter = map_iter(cluster);
            while ((key = map_next(cluster, &node_iter))) {
                node* n = map_get(cluster, key);
                if (n) {
                    log_debug("node deinit: %s", n->id);
                    node_deinit(n);
                }
            }
            map_deinit(cluster);
        }
    }
    map_deinit(&nc->map_cluster);
    //pid、thread_run_flag自动释放。
    pthread_mutex_destroy(&(nc->file_mutex));
    free(nc);
}
//运行node_center线程
void node_center_run(void* arg)
{
    node_center* nc = arg;
    nc->thread_run_flag = THREAD_RUNING;
    
    while (THREAD_RUNING == nc->thread_run_flag) {
        check_node_timeout(nc);
        // rpc_check_timeouts(nc->rpc_ctx);
        sleep(1);
    }
}
//创建并运行node_center线程
void node_center_run_thread(node_center* nc)
{
    if (NULL == nc)
    {
        return;
    }
    
    pthread_create(&nc->pid, NULL, (void*)node_center_run, (void*)nc);
}
//停止node_center线程
void node_center_stop(node_center* nc)
{
    nc->thread_run_flag = THREAD_STOPING;
    pthread_join(nc->pid, NULL);
}