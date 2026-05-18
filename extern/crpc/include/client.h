#ifndef CRPC_CLIENT_H
#define CRPC_CLIENT_H

#include "context.h"
#include "net.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RpcClient RpcClient;
typedef int (*ClientOnTransact)(RpcClient *client, Context *context, void *arg);


/*
 * RpcClient 结构定义
 * 客户端发送请求后等待服务端应答，以及处理服务端通知事件。
 * 所以，客户端中通过线程接收服务端消息，判断消息类型后再处理。
 */
struct RpcClient {
    Context *context;
    int threadRunFlag;
    pthread_t threadId;
    int waitReply;
    pthread_mutex_t mutex;
    pthread_cond_t condW;
    int callLockFlag;
    pthread_mutex_t lockMutex;
    pthread_cond_t lockCond;
    void* arg; // ClientOnTransact的第三个参数传参，可自定义
    ClientOnTransact clientOnTransact;
    
};

RpcClient *CreateRpcClient(const char *ip, uint16_t port);
void ReleaseRpcClient(RpcClient *client);
void LockRpcClient(RpcClient *client);
void UnlockRpcClient(RpcClient *client);
int RemoteCall(RpcClient *client);
void ReadClientEnd(RpcClient *client);
void SetCallbackClientOnTransact(RpcClient *client, ClientOnTransact cb);

#ifdef __cplusplus
}
#endif
#endif