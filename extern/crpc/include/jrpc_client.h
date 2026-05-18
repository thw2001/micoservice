#ifndef JRPC_CLIENT_H
#define JRPC_CLIENT_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "client.h"

typedef RpcClient JrpcClient;

JrpcClient* CreateJrpcClient(const char *ip, uint16_t port);

char* JrpcCallFunc(JrpcClient *client, char* jsonrpc_request);


#ifdef __cplusplus
}
#endif
#endif // JRPC_CLIENT_H