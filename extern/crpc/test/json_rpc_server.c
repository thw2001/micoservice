#include "server.h"
#include "serial.h"
#include "log.h"
#include "jrpc_server.h"
// gcc rpc_server.c -I../include -L../src -lrpcserver -o server

#undef LOG_TAG
#define LOG_TAG "JSON_RPC_SERVER"

cJSON * say_hello(JrpcContext * ctx, cJSON * params, cJSON *id) {
	return cJSON_CreateString("Hello!");
}


cJSON * add(JrpcContext * ctx, cJSON * params, cJSON *id) {
	cJSON * a = cJSON_GetArrayItem(params,0);
	cJSON * b = cJSON_GetArrayItem(params,1);
	return cJSON_CreateNumber(a->valueint + b->valueint);
}

int main(void)
{
    JrpcServer *server = CreateJrpcServer("127.0.0.1", 19977);
    if (server == NULL) {
        LOGE("Create rpc server failed!");
        return -1;
    }
    JrpcRegisterProcedure(server, say_hello, "sayHello", NULL);
    JrpcRegisterProcedure(server, add, "add", NULL);
    RunJrpcLoop(server);
    ReleaseJrpcServer(server);
    LOGD("Server exited");
    return 0;
}