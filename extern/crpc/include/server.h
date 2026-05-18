#ifndef CRPC_SERVER_H
#define CRPC_SERVER_H

#include "common.h"
#include "context.h"
#include "evloop.h"
#include "hash_table.h"
#include "net.h"
#include "serial.h"


#ifdef __cplusplus
extern "C" {
#endif

struct Node {
    Context *context;
    struct Node *next;
};

// 事件注册
struct EventNode {
    int event;
    struct Node *head;
};

typedef struct RpcServer RpcServer;

typedef int (*ServerOnTransact)(void *server, Context *context);
typedef int (*ServerOnCallbackTransact)(void *server, int event, Context *context);
typedef int (*ServerEndCallbackTransact)(void *server, int event);

typedef struct RpcServer {
    int listenFd;
    EventLoop *loop;
    HashTable *clients;
    pthread_mutex_t mutex;
    int events[100];
    int nEvents;
    struct EventNode eventNode[100];
    void* arg; // 这个参数是以下三个回调的第一个传参，在创建RpcServer的对象时，默认指向自己
    ServerOnTransact serverOnTransact;
    ServerOnCallbackTransact serverOnCallbackTransact;
    ServerEndCallbackTransact serverEndCallbackTransact;

    
} RpcServer;

RpcServer *CreateRpcServer(const char *ip, uint16_t port);
int RunRpcLoop(RpcServer *server);
void ReleaseRpcServer(RpcServer *server);
int RegisterCallback(RpcServer *server, int event, Context *context);
int UnRegisterCallback(RpcServer *server, int event, const Context *context);
int EmitEvent(RpcServer *server, int event);
// int OnTransact(RpcServer *server, Context *context);
// int OnCallbackTransact(const RpcServer *server, int event, Context *context);
// int EndCallbackTransact(const RpcServer *server, int event);

void SetCallbackServerOnTransact(RpcServer *server, ServerOnTransact cb);
void SetCallbackServerOnCallbackTransact(RpcServer *server, ServerOnCallbackTransact cb);
void SetCallbackServerEndCallbackTransact(RpcServer *server, ServerEndCallbackTransact cb);

int RpcRegisterEventCallback(RpcServer *server, Context *context);
int RpcUnRegisterEventCallback(RpcServer *server, Context *context);

#ifdef __cplusplus
}
#endif
#endif