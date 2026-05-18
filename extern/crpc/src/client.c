#include "client.h"
#include <signal.h>
#include <sys/time.h>
#include "log.h"

#undef LOG_TAG
#define LOG_TAG "CRpcClient"

const int CLIENT_STATE_IDLE = 0;
const int CLIENT_STATE_DEAL_REPLY = 1;
const int CLIENT_STATE_EXIT = 2;

static char *RpcClientReadMsg(RpcClient *client)
{
    if (client == NULL) {
        return NULL;
    }

    char *buff = ContextGetReadRecord(client->context);
    while (buff == NULL && client->threadRunFlag) {
        int ret = WaitFdEvent(client->context->fd, READ_EVENT, 1000);
        if (ret < 0) {
            LOGE("wait server reply message failed!");
            client->threadRunFlag = 0;
            return NULL;
        } else if (ret == 0) {
            continue;
        }
        ret = ContextReadNet(client->context);
        if (ret < 0) {
            LOGE("read server reply message failed!");
            client->threadRunFlag = 0;
            return NULL;
        }
        buff = ContextGetReadRecord(client->context);
    }
    if (!client->threadRunFlag) {
        if (buff != NULL) {
            free(buff);
        }
        return NULL;
    }
    return buff;
}

static void RpcClientDealReadMsg(RpcClient *client, char *buff)
{
    if (client == NULL) {
        return;
    }

    char szTmp[8] = {0};
    snprintf(szTmp, sizeof(szTmp), "N%c", client->context->cSplit);
    if (strncmp(buff, szTmp, strlen(szTmp)) == 0) { /* deal reply message */
        pthread_mutex_lock(&client->mutex);
        client->waitReply = CLIENT_STATE_DEAL_REPLY;
        client->context->oneProcess = buff;
        client->context->nPos = strlen(szTmp);
        client->context->nSize = strlen(buff);
        pthread_cond_signal(&client->condW);
        pthread_mutex_unlock(&client->mutex);
    } else { /* deal callback message */
        pthread_mutex_lock(&client->mutex);
        while (client->waitReply == CLIENT_STATE_DEAL_REPLY) {
            pthread_cond_wait(&client->condW, &client->mutex);
        }
        pthread_mutex_unlock(&client->mutex);
        client->context->oneProcess = buff;
        client->context->nPos = strlen(szTmp);
        client->context->nSize = strlen(buff);
        if (client->clientOnTransact != NULL)
        {
            client->clientOnTransact(client, client->context, client->arg);
        }
        free(buff);
    }
    return;
}

static void *RpcClientThreadDeal(void *arg)
{
    RpcClient *client = (RpcClient *)arg;
    if (client == NULL) {
        return NULL;
    }

    while (client->threadRunFlag) {
        char *buff = RpcClientReadMsg(client);
        if (buff == NULL) {
            continue;
        }
        RpcClientDealReadMsg(client, buff);
    }
    pthread_mutex_lock(&client->mutex);
    client->waitReply = CLIENT_STATE_EXIT;
    pthread_cond_signal(&client->condW);
    pthread_mutex_unlock(&client->mutex);
    LOGI("Client read message thread exiting!");
    return NULL;
}

RpcClient *CreateRpcClient(const char *ip, uint16_t port)
{
    int fd = ConnectUnixServer(ip, port);
    if (fd < 0) {
        return NULL;
    }
    SetNonBlock(fd, 1);
    RpcClient *client = (RpcClient *)calloc(1, sizeof(RpcClient));
    client->clientOnTransact = NULL;
    client->arg = NULL;
    if (client == NULL) {
        close(fd);
        return NULL;
    }
    client->context = CreateContext(CONTEXT_BUFFER_MIN_SIZE);
    if (client->context == NULL) {
        close(fd);
        free(client);
        return NULL;
    }
    client->context->fd = fd;
    client->threadRunFlag = 1;
    client->threadId = 0;
    client->waitReply = CLIENT_STATE_IDLE;
    client->callLockFlag = 0;
    pthread_mutex_init(&client->mutex, NULL);
    pthread_cond_init(&client->condW, NULL);
    pthread_mutex_init(&client->lockMutex, NULL);
    pthread_cond_init(&client->lockCond, NULL);
    int ret = pthread_create(&client->threadId, NULL, RpcClientThreadDeal, client);
    if (ret) {
        pthread_cond_destroy(&client->condW);
        pthread_mutex_destroy(&client->mutex);
        pthread_cond_destroy(&client->lockCond);
        pthread_mutex_destroy(&client->lockMutex);
        ReleaseContext(client->context);
        close(fd);
        free(client);
        return NULL;
    }
    signal(SIGPIPE, SIG_IGN);
    return client;
}

void ReleaseRpcClient(RpcClient *client)
{
    if (client != NULL) {
        if (client->threadId != 0) {
            client->threadRunFlag = 0;
            pthread_join(client->threadId, NULL);
        }
        pthread_cond_destroy(&client->condW);
        pthread_mutex_destroy(&client->mutex);
        pthread_cond_destroy(&client->lockCond);
        pthread_mutex_destroy(&client->lockMutex);
        close(client->context->fd);
        ReleaseContext(client->context);
        free(client);
    }
    return;
}

int RemoteCall(RpcClient *client)
{
    if (client == NULL) {
        return -1;
    }
    if (client->waitReply == CLIENT_STATE_EXIT) {
        return -1;
    }
    int ret = 0;
    Context *context = client->context;
    while (context->wBegin != context->wEnd && ret >= 0) {
        ret = ContextWriteNet(context);
    }
    if (ret < 0) {
        return ret;
    }
    ret = 0; /* reset ret value */
    pthread_mutex_lock(&client->mutex);
    while (client->waitReply != CLIENT_STATE_DEAL_REPLY && client->waitReply != CLIENT_STATE_EXIT) {
        pthread_cond_wait(&client->condW, &client->mutex);
    }
    if (client->waitReply == CLIENT_STATE_EXIT) {
        ret = -1;
    }
    pthread_mutex_unlock(&client->mutex);
    return ret;
}

void ReadClientEnd(RpcClient *client)
{
    if (client == NULL) {
        return;
    }

    pthread_mutex_lock(&client->mutex);
    free(client->context->oneProcess);
    client->context->oneProcess = NULL;
    if (client->waitReply == CLIENT_STATE_DEAL_REPLY) {
        client->waitReply = CLIENT_STATE_IDLE;
    }
    pthread_cond_signal(&client->condW);
    pthread_mutex_unlock(&client->mutex);
    return;
}

void LockRpcClient(RpcClient *client)
{
    if (client == NULL) {
        return;
    }

    pthread_mutex_lock(&client->lockMutex);
    while (client->callLockFlag != 0) {
        pthread_cond_wait(&client->lockCond, &client->lockMutex);
    }
    client->callLockFlag = 1;
    pthread_mutex_unlock(&client->lockMutex);
    return;
}

void UnlockRpcClient(RpcClient *client)
{
    if (client == NULL) {
        return;
    }

    pthread_mutex_lock(&client->lockMutex);
    client->callLockFlag = 0;
    pthread_cond_signal(&client->lockCond);
    pthread_mutex_unlock(&client->lockMutex);
    return;
}

void SetCallbackClientOnTransact(RpcClient *client, ClientOnTransact cb)
{
    client->clientOnTransact = cb;
}