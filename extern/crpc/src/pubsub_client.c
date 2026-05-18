#include "serial.h"
#include "om.h"
#include "pubsub_client.h"
#include "log.h"
#include "pubsub_base.h"

int CrpcPubSubClientOnTransact(RpcClient *client, Context *context, void* arg)
{
    PubSubClient* psclient = arg;
    int payload_len = -1, ret = -1;
    ret = ReadInt(context, &payload_len);
    if (ret < 0 || payload_len < 0) {
        return READ_PAYLOAD_LEN_FAILED;
    }

    char topic_name[PUBSUB_TOPIC_NAME_LEN] = {0};
    ret = ReadStr(context, topic_name, PUBSUB_TOPIC_NAME_LEN);
    if (ret < 0)
    {
        return READ_TOPIC_FAILED;
    }
    if (ret > 0)
    {
        return TOPIC_NAME_LEN_TOOLONG_FAILED;
    }

    uint8_t* tmp = (uint8_t*)calloc(payload_len + 1, sizeof(uint8_t));
    ret = ReadUStr(context, tmp, payload_len + 1);
    if (ret < 0)
    {
        free(tmp);
        return READ_PAYLOAD_FAILED;
    }

    if (ret > 0)
    {
        free(tmp);
        return READ_PAYLOAD_BUF_TOOLONG_FAILED;
    }
    
    // LOGD("recv:%s %s %d", topic_name, tmp, payload_len);
    // int i = 0;
    // printf("bin:\n");
    // for (i = 0; i < payload_len; i++)
    // {
    //     printf("%02x ", tmp[i]);
    // }
    // printf("\nend\n");

    if (psclient->onMessage != NULL)
        psclient->onMessage(psclient->arg, topic_name, tmp, payload_len);

    free(tmp);
    return CRPC_SUCCESS;
}

void CrpcPubSubOnMessageCB(PubSubClient *client, SubOnMessage callback)
{
    client->onMessage = callback;
}

PubSubClient* CreateCrpcPubSubClient(char *ip, uint16_t port)
{
    PubSubClient *psclient = malloc(sizeof(PubSubClient));
    psclient->arg = psclient;
    psclient->client = CreateRpcClient(ip, port);
    psclient->client->arg = psclient;
    if (psclient->client)
    {
        /* code */
        // client->clientOnTransact = NULL;
        SetCallbackClientOnTransact(psclient->client, CrpcPubSubClientOnTransact);
        return psclient;
    }
    
    return NULL;
}

int CrpcSub(PubSubClient* psclient, char* topic)
{
    RpcClient* client = psclient->client;
    if (client == NULL || topic == NULL)
    {
        return CRPC_FAILED;
    }
    
    LockRpcClient(client);
    Context *context = client->context;
    WriteBegin(context, 0);
    WriteInt(context, PUBSUB_CMD_SUB);
    WriteStr(context, topic);
    WriteEnd(context);

    int ret = RemoteCall(client);
    if (ret < 0) {
        LOGE("PubSubClient remote call failed!");
        UnlockRpcClient(client);
        return CRPC_FAILED;
    }
    int brokerRet = DEFAULT_FAILED_VALUE;
    int len = ReadInt(context, &brokerRet);
    if (len < 0) {
        UnlockRpcClient(client);
        return CRPC_FAILED;
    }
    
    ReadClientEnd(client);
    UnlockRpcClient(client);
    return brokerRet;
}

int CrpcUnSub(PubSubClient* psclient, char* topic)
{
    if (psclient == NULL || topic == NULL)
    {
        return CRPC_FAILED;
    }
    RpcClient* client = psclient->client;
    LockRpcClient(client);
    Context *context = client->context;
    WriteBegin(context, 0);
    WriteInt(context, PUBSUB_CMD_UNSUB);
    WriteStr(context, topic);
    WriteEnd(context);

    int ret = RemoteCall(client);
    if (ret < 0) {
        LOGE("PubSubClient remote call failed!");
        UnlockRpcClient(client);
        return CRPC_FAILED;
    }
    int brokerRet = DEFAULT_FAILED_VALUE;
    int len = ReadInt(context, &brokerRet);
    if (len < 0) {
        UnlockRpcClient(client);
        return CRPC_FAILED;
    }
    
    ReadClientEnd(client);
    UnlockRpcClient(client);
    return brokerRet;
}

int CrpcPubMessage(PubSubClient* psclient, char* topic, uint8_t* payload, uint32_t payload_len)
{
    RpcClient* client = psclient->client;
    LockRpcClient(client);
    Context *context = client->context;
    WriteBegin(context, 0);
    WriteInt(context, PUBSUB_CMD_PUB);
    WriteStr(context, topic);
    WriteInt(context, payload_len);
    WriteUStr(context, payload, payload_len);
    WriteEnd(context);

    int ret = RemoteCall(client);
    if (ret < 0) {
        LOGE("PubSubClient remote call failed!");
        UnlockRpcClient(client);
        return CRPC_FAILED;
    }
    int brokerRet = DEFAULT_FAILED_VALUE;
    int len = ReadInt(context, &brokerRet);
    if (len < 0) {
        UnlockRpcClient(client);
        return CRPC_FAILED;
    }
    
    ReadClientEnd(client);
    UnlockRpcClient(client);
    return brokerRet;
}

void ReleaseCrpcPubSubClient(PubSubClient* psclient)
{
    ReleaseRpcClient(psclient->client);
    free(psclient);
    psclient = NULL;
}
