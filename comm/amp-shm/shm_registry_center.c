#include "abs_comm.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "om.h"
#include "pubsub.h"
#include "vec.h"
#include "base_types.h"
#include "mico_service.h"
#include "cf_log.h"

#include <sys/shm.h>
#define SHM_KEY 1236

typedef struct registry_subers
{
    om_suber_t* suber;
    om_topic_t* om_topic;
    sub_topic_callback recv_cb;
    void* arg;
}registry_subers_t;

typedef vec_t(registry_subers_t*) vec_registry_subers_t;

typedef struct shm_center_client
{
    vec_registry_subers_t vrs;
    pubsub_config_new_t pcfg;
    pthread_t pid; // 发布订阅接收消息主线程
    pthread_mutex_t pub_mutex;
}shm_center_client_t;


int registry_send_str_to_micoservice(abs_comm* arg, char* micreservice, void* msg, uint32_t len)
{
    abs_comm* rs = arg;
    pthread_mutex_lock(&(((shm_center_client_t*)(rs->client))->pub_mutex));
    int ret = pub_msg(&(((shm_center_client_t*)(rs->client))->pcfg), rs->client_name, micreservice, msg, len);
    pthread_mutex_unlock(&(((shm_center_client_t*)(rs->client))->pub_mutex));
    return ret;
}

int shm_mem_config(pubsub_config_new_t* pcfg)
{
    int shmid = -1;
    void* shm_addr = NULL;
    const char* dev_files[] = {
        FS_BASE"microservice-config.json",
        FS_BASE"node-config.json",
    };
    pub_init_config_files(pcfg, dev_files, 2);
    int mem = pubsub_cal_mem(pcfg);
    // 注册信号处理函数
#if defined(__SHM_USE_SHMGET__)
    // 创建共享内存
    shmid = shmget(SHM_KEY, mem, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(1);
    }
    // 附加共享内存
    shm_addr = shmat(shmid, NULL, 0);
    if (shm_addr == (void*)-1) {
        perror("shmat failed");
        exit(1);
    }
#elif defined(__SHM_USE_ADDR__)
    //用户态虚拟地址空间0x10000000 - 0x20000000，中间件的共享内存从0x10400000开始动态分配
    //shm_addr = (void*)0x10000000UL;
    shm_addr = (void*)AMP_SHM_START;
#else
#error "no shm alloc func"
#endif
    
    int ret = pubsub_init(pcfg, 0, shm_addr, mem);

    log_debug("ret: %d", ret);
    return ret;
}

void* shm_loop_recv(void* arg)
{
    abs_comm* rc = arg;

    recv_msg(&(((shm_center_client_t*)(rc->client))->pcfg), rc->client_name);
}

abs_comm *abs_comm_new(char* client_name, abs_comm_connect *rsc)
{
    abs_comm* rs = malloc(sizeof(abs_comm));

    shm_center_client_t* sc_client = malloc(sizeof(shm_center_client_t));

    // vec_registry_subers_t* vec_subers = malloc(sizeof(vec_registry_subers_t));

    if (NULL == sc_client)
    {
        free(rs);
        return NULL;
    }

    pthread_mutex_init(&sc_client->pub_mutex, NULL);

    vec_init(&sc_client->vrs);
    pubsub_config_new_init(&sc_client->pcfg);

    shm_mem_config(&sc_client->pcfg);

    (rs->client) = sc_client;

    rs->pub_msg = registry_send_str_to_micoservice;
    rs->client_name = strdup(client_name);

    pthread_create(&sc_client->pid, NULL, shm_loop_recv, (void*) rs);
    
    return rs;
}

int center_suber_on_message(om_msg_t* msg, void* arg)
{
    registry_subers_t* rsb = arg;
    if (NULL != rsb && NULL != rsb->recv_cb)
    {
        rsb->recv_cb(rsb->arg, msg->buff, msg->size);
    }
    
}

int abs_comm_sub(abs_comm * rc, char* topic_name, void *arg, sub_topic_callback recv_cb)
{
    om_topic_t *topic = om_core_find_topic(topic_name, 0);
    if (topic == NULL)
    {
        return BASE_FAILED;
    }

    om_suber_t* suber1 = malloc(sizeof(om_suber_t));
    registry_subers_t* rsb = malloc(sizeof(registry_subers_t));
    rsb->om_topic = topic;
    rsb->suber = suber1;
    rsb->recv_cb = recv_cb;
    rsb->arg = arg;

    om_config_suber(suber1, "dt", center_suber_on_message, rsb, topic);

    if (NULL == suber1)
    {
        return BASE_FAILED;
    }

    vec_push(&(((shm_center_client_t*)(rc->client))->vrs), rsb);

    return BASE_SUCCESS;
}


void abs_comm_delete(abs_comm *rs)
{
    // free rs.client
    if (NULL == rs)
    {
        return;
    }
    shm_center_client_t* sc_client = rs->client;

    pthread_cancel(sc_client->pid); // 先直接cancel

    pubsub_config_new_deinit(&(((shm_center_client_t*)(rs->client))->pcfg));

    vec_registry_subers_t* vrs = &sc_client->vrs;
    int i = 0;
    registry_subers_t* rsb;

    vec_foreach(vrs, rsb, i)
    {
        OM_TOPIC_LOCK(rsb->om_topic);
        om_msg_del_suber(rsb->suber);
        OM_TOPIC_UNLOCK(rsb->om_topic);
        free(rsb);
    }

    vec_deinit(vrs);

    free(vrs);
    free(rs->client_name);
    free(rs);
}
