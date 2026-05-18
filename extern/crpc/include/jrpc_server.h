#ifndef JRPC_SERVER_H
#define JRPC_SERVER_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "cJSON.h"
#include "server.h"

#define JRPC_PARSE_ERROR -32700
#define JRPC_INVALID_REQUEST -32600
#define JRPC_METHOD_NOT_FOUND -32601
#define JRPC_INVALID_PARAMS -32603
#define JRPC_INTERNAL_ERROR -32693

/********************************* json rpc *********************************/
typedef struct {
	void *data;
	int error_code;
	char * error_message;
} JrpcContext;

typedef cJSON* (*JrpcFunc)(JrpcContext *context, cJSON *params, cJSON* id);

typedef struct jrpc_procedure {
	char * name;
	JrpcFunc function;
	void *data;
} JrpcProcedure;
/********************************* json rpc end *********************************/

typedef struct JrpcServer{
    RpcServer* server;
    int procedure_count;
	JrpcProcedure *procedures;
}JrpcServer;

int JrpcRegisterProcedure(JrpcServer *server, JrpcFunc function_pointer, char *name, void *data);

int JrpcDeregisterProcedure(JrpcServer *server, char *name);

JrpcServer *CreateJrpcServer(const char *ip, uint16_t port);

int RunJrpcLoop(JrpcServer *server);

void ReleaseJrpcServer(JrpcServer *server);

#ifdef __cplusplus
}
#endif
#endif // JRPC_SERVER_H

