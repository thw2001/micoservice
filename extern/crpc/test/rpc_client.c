#include "client.h"
#include "serial.h"
#include "log.h"

// gcc rpc_client.c -I../include -L../src -lrpcclient -lpthread -o client

#undef LOG_TAG
#define LOG_TAG "RPC_CLIENT"

RpcClient *client = NULL;

int OnTransact(RpcClient *client, Context *context, void *arg)
{
    return 0;
}

void echo(void)
{
    LockRpcClient(client);
    Context *context = client->context;
    WriteBegin(context, 0);
    WriteFunc(context, "echo");
    WriteStr(context, "Hello, world");
    WriteEnd(context);
    int ret = RemoteCall(client);
    if (ret < 0) {
        LOGE("remote call echo failed!");
        UnlockRpcClient(client);
        return;
    }
    int len = ReadStr(context, NULL, 0);
    if (len < 0) {
        UnlockRpcClient(client);
        return;
    }
    char *pstr = (char *)calloc(len + 1, sizeof(char));
    if (pstr == NULL) {
        UnlockRpcClient(client);
        return;
    }
    ReadStr(context, pstr, len + 1);
    ReadClientEnd(client);
    UnlockRpcClient(client);
    LOGD("Server return [%s]", pstr);
    free(pstr);
    return;
}

int main(void)
{
    client = CreateRpcClient("127.0.0.1", 19977);
    client->clientOnTransact = OnTransact;
    if (client == NULL) {
        LOGE("Create rpc client failed!");
        return -1;
    }
    echo();
    return 0;
}