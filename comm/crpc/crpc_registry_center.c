#include "abs_comm.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "pubsub_client.h"
#include "map.h"
#include "pthread.h"
#include "unistd.h"
#include "time.h"
#include "sys/time.h"

// 定义消息队列条目结构
typedef struct message_queue_entry {
    char* topic;
    void* payload;
    uint32_t payload_len;
    struct message_queue_entry* next;
} message_queue_entry;

// 定义主题回调条目结构
typedef struct topic_callback_entry {
    void* arg;
    sub_topic_callback recv_cb;
} topic_callback_entry;

// 定义扩展的客户端结构体，包含原始的PubSubClient和主题回调映射
typedef struct crpc_registry_client {
    PubSubClient* pubsub_client;  // 原始的PubSubClient
    map_t(topic_callback_entry) topic_callbacks;  // 主题回调映射
    
    // 消息队列相关
    message_queue_entry* message_queue_head;
    message_queue_entry* message_queue_tail;
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;
    pthread_t worker_thread;
    int worker_running;
} crpc_registry_client;

// 消息队列条目清理函数
static void free_message_queue_entry(message_queue_entry* entry) {
    if (entry) {
        if (entry->topic) {
            free(entry->topic);
        }
        if (entry->payload) {
            free(entry->payload);
        }
        free(entry);
    }
}

// 工作线程函数
static void* worker_thread_func(void* arg) {
    abs_comm* rc = (abs_comm*)arg;
    if (NULL == rc || NULL == rc->client) {
        return NULL;
    }

    crpc_registry_client* client = (crpc_registry_client*)rc->client;
    
    while (client->worker_running) {
        // 等待队列中有消息
        pthread_mutex_lock(&client->queue_mutex);
        while (client->worker_running && client->message_queue_head == NULL) {
            pthread_cond_wait(&client->queue_cond, &client->queue_mutex);
        }
        
        // 取出队列中的消息
        message_queue_entry* entry = client->message_queue_head;
        if (entry != NULL) {
            client->message_queue_head = entry->next;
            if (client->message_queue_head == NULL) {
                client->message_queue_tail = NULL;
            }
        }
        pthread_mutex_unlock(&client->queue_mutex);
        
        // 发送消息
        if (entry != NULL && client->pubsub_client != NULL) {
            CrpcPubMessage(client->pubsub_client, entry->topic, entry->payload, entry->payload_len);
            free_message_queue_entry(entry);
        }
    }
    
    return NULL;
}

// 消息回调函数
static int registry_sub_on_message(void *arg, char* topic, uint8_t* payload, uint32_t len)
{
    abs_comm* rc = arg;
    if (NULL == rc) {
        return -1;
    }

    // 获取扩展的客户端结构体
    crpc_registry_client* client = (crpc_registry_client*)rc->client;
    if (NULL == client || NULL == client->pubsub_client) {
        return -1;
    }

    // 在主题回调映射中查找具体的主题回调
    topic_callback_entry* entry = map_get(&client->topic_callbacks, topic);
    if (entry != NULL && entry->recv_cb != NULL) {
        // 调用特定主题的回调
        return entry->recv_cb(entry->arg, payload, len);
    }

    // 如果没有找到特定主题的回调，则返回错误或使用默认处理
    return -1;
}

// 发送消息到微服务的函数
int registry_send_str_to_micoservice(abs_comm* arg, char* micreservice, void* msg, uint32_t len)
{
    abs_comm* rc = arg;
    if (NULL == rc || NULL == rc->client) {
        return -1;
    }

    crpc_registry_client* client = (crpc_registry_client*)rc->client;
    if (NULL == client) {
        return -1;
    }

    // 创建消息队列条目
    message_queue_entry* entry = malloc(sizeof(message_queue_entry));
    if (NULL == entry) {
        return -1;
    }

    entry->topic = strdup(micreservice);
    if (NULL == entry->topic) {
        free(entry);
        return -1;
    }

    entry->payload = malloc(len);
    if (NULL == entry->payload) {
        free(entry->topic);
        free(entry);
        return -1;
    }

    memcpy(entry->payload, msg, len);
    entry->payload_len = len;
    entry->next = NULL;

    // 将消息添加到队列
    pthread_mutex_lock(&client->queue_mutex);
    if (client->message_queue_tail != NULL) {
        client->message_queue_tail->next = entry;
    } else {
        client->message_queue_head = entry;
    }
    client->message_queue_tail = entry;
    pthread_mutex_unlock(&client->queue_mutex);

    // 通知工作线程有新消息
    pthread_cond_signal(&client->queue_cond);

    return 0;
}

// 创建abs_comm实例
abs_comm *abs_comm_new(char* client_name, abs_comm_connect* rsc)
{
    if (NULL == rsc) {
        return NULL;
    }

    abs_comm* rc = malloc(sizeof(abs_comm));
    if (NULL == rc) {
        return NULL;
    }

    // 创建扩展的客户端结构体
    crpc_registry_client* client = malloc(sizeof(crpc_registry_client));
    if (NULL == client) {
        free(rc);
        return NULL;
    }

    // 初始化主题回调映射
    map_init(&client->topic_callbacks);

    // 初始化消息队列
    client->message_queue_head = NULL;
    client->message_queue_tail = NULL;
    pthread_mutex_init(&client->queue_mutex, NULL);
    pthread_cond_init(&client->queue_cond, NULL);
    client->worker_running = 1;

    // 创建原始的PubSubClient
    client->pubsub_client = CreateCrpcPubSubClient(rsc->ip, rsc->port);
    if (NULL == client->pubsub_client) {
        map_deinit(&client->topic_callbacks);
        pthread_mutex_destroy(&client->queue_mutex);
        pthread_cond_destroy(&client->queue_cond);
        free(client);
        free(rc);
        return NULL;
    }

    // 设置回调参数
    client->pubsub_client->arg = rc;
    // CrpcSub(client->pubsub_client, register_id);
    CrpcPubSubOnMessageCB(client->pubsub_client, registry_sub_on_message);

    // 启动工作线程
    if (pthread_create(&client->worker_thread, NULL, worker_thread_func, rc) != 0) {
        ReleaseCrpcPubSubClient(client->pubsub_client);
        map_deinit(&client->topic_callbacks);
        pthread_mutex_destroy(&client->queue_mutex);
        pthread_cond_destroy(&client->queue_cond);
        free(client);
        free(rc);
        return NULL;
    }

    // 设置abs_comm的字段
    rc->client = client;
    rc->client_name = strdup(client_name);
    rc->pub_msg = registry_send_str_to_micoservice;

    return rc;
}

// 为特定主题注册回调函数
int abs_comm_sub(abs_comm * rc, char* topic_name, void *arg, sub_topic_callback recv_cb)
{
    if (NULL == rc || NULL == topic_name || NULL == rc->client) {
        return -1;
    }

    crpc_registry_client* client = (crpc_registry_client*)rc->client;
    if (NULL == client || NULL == client->pubsub_client) {
        return -1;
    }

    // 创建主题回调条目
    topic_callback_entry entry;
    entry.arg = arg;
    entry.recv_cb = recv_cb;

    // 将主题和回调添加到映射中
    map_set(&client->topic_callbacks, topic_name, entry);

    // 订阅该主题
    return CrpcSub(client->pubsub_client, topic_name);
}

// 删除abs_comm实例
void abs_comm_delete(abs_comm *rc)
{
    if (NULL == rc) {
        return;
    }

    if (rc->client != NULL) {
        crpc_registry_client* client = (crpc_registry_client*)rc->client;
        
        // 停止工作线程
        client->worker_running = 0;
        pthread_cond_signal(&client->queue_cond);
        pthread_join(client->worker_thread, NULL);
        
        // 清理消息队列
        pthread_mutex_lock(&client->queue_mutex);
        while (client->message_queue_head != NULL) {
            message_queue_entry* entry = client->message_queue_head;
            client->message_queue_head = entry->next;
            free_message_queue_entry(entry);
        }
        pthread_mutex_unlock(&client->queue_mutex);
        
        // 销毁同步对象
        pthread_mutex_destroy(&client->queue_mutex);
        pthread_cond_destroy(&client->queue_cond);
        
        // 释放PubSubClient
        if (client->pubsub_client != NULL) {
            ReleaseCrpcPubSubClient(client->pubsub_client);
        }
        
        // 清理主题回调映射
        map_deinit(&client->topic_callbacks);
        
        // 释放扩展的客户端结构体
        free(client);
    }

    if (rc->client_name) {
        free(rc->client_name);
    }

    free(rc);
}
