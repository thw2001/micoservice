#include "server.h"
#include "serial.h"
#include "log.h"

// gcc rpc_server.c -I../include -L../src -lrpcserver -o server

#undef LOG_TAG
#define LOG_TAG "RPC_SERVER"

int Echo(RpcServer *server, Context *context)
{
    int len = ReadStr(context, NULL, 0);
    if (len < 0) {
        return -1;
    }
    char *pstr = (char *)calloc(len + 1, sizeof(char));
    if (pstr == NULL) {
        return -1;
    }
    ReadStr(context, pstr, len + 1);
    WriteBegin(context, 0);
    WriteStr(context, pstr);
    WriteEnd(context);
    free(pstr);
    return 0;
}

int OnTransact(void *server, Context *context)
{
    char func[128] = {0};
    int ret = ReadFunc(context, func, 128);
    if (ret != 0) {
        return -1;
    }
    ret = 0;
    if (strcmp(func, "echo") == 0) {
        Echo(server, context);
    } else {
        WriteBegin(context, 0);
        WriteStr(context, "unsupport function");
        WriteEnd(context);
    }
    return 0;
}

int OnCallbackTransact(void *server, int event, Context *context)
{
    printf("OnEvent:%d\n", event);
    return 0;
}

int EndCallbackTransact(void *server, int event)
{
    printf("EndEvent:%d\n", event);
    return 0;
}

int main(void)
{
    RpcServer *server = CreateRpcServer("127.0.0.1", 19977);
    server->serverOnTransact = OnTransact;
    server->serverOnCallbackTransact = OnCallbackTransact;
    server->serverEndCallbackTransact = EndCallbackTransact;
    if (server == NULL) {
        LOGE("Create rpc server failed!");
        return -1;
    }
    RunRpcLoop(server);
    ReleaseRpcServer(server);
    unlink("./test.sock");
    LOGD("Server exited");
    return 0;
}