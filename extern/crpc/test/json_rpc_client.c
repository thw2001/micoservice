#include "jrpc_client.h"
#include "serial.h"
#include "log.h"

// gcc rpc_client.c -I../include -L../src -lrpcclient -lpthread -o client

#undef LOG_TAG
#define LOG_TAG "JSON_RPC_CLIENT"


int main(void)
{
    JrpcClient* client = CreateJrpcClient("127.0.0.1", 19977);
    if (client == NULL) {
        LOGE("Create rpc client failed!");
        return -1;
    }
    char* tmp;
    tmp = JrpcCallFunc(client, "{\"method\":\"sayHello\", \"id\": \"12\"}");
    free(tmp);
    tmp = JrpcCallFunc(client, "{\"method\": \"add\", \"params\": [3,4], \"id\": \"11\" }");
    free(tmp);
    return 0;
}