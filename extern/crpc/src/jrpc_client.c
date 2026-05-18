#include "jrpc_client.h"
#include "serial.h"
#include "log.h"

#undef LOG_TAG
#define LOG_TAG "JrpcClient"

JrpcClient *CreateJrpcClient(const char *ip, uint16_t port)
{
    JrpcClient *client = CreateRpcClient(ip, port);
    if (client)
    {
        /* code */
        client->clientOnTransact = NULL;
        return client;
    }
    
    return NULL;
}

/**
 * @brief jrpc远程调用函数
 * 
 * @param client jrpc client句柄
 * @param jsonrpc_request 符合json rpc规范的json请求
 * @return char* 返回的结果json, 需要手动释放
 */
char* JrpcCallFunc(JrpcClient *client, char* jsonrpc_request)
{
    LockRpcClient(client);
    Context *context = client->context;
    WriteBegin(context, 0);
    WriteStr(context, jsonrpc_request);
    WriteEnd(context);
    int ret = RemoteCall(client);
    if (ret < 0) {
        LOGE("Jrpc client remote call failed!");
        UnlockRpcClient(client);
        return NULL;
    }
    int len = ReadStr(context, NULL, 0);
    if (len < 0) {
        UnlockRpcClient(client);
        return NULL;
    }
    char *pstr = (char *)calloc(len + 1, sizeof(char));
    if (pstr == NULL) {
        UnlockRpcClient(client);
        return NULL;
    }
    ReadStr(context, pstr, len + 1);
    ReadClientEnd(client);
    UnlockRpcClient(client);
    LOGD("Server return len:[%d] [%s]", len+1, pstr);
    // free(pstr);
    return pstr;
}