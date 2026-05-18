#ifndef PUBSUB_CLIENT_H
#define PUBSUB_CLIENT_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "client.h"

typedef int (*SubOnMessage)(void *arg, char* topic, uint8_t* payload, uint32_t len);

typedef struct PubSubClient{
    RpcClient* client;
    SubOnMessage onMessage;
    void *arg;
}PubSubClient;

PubSubClient* CreateCrpcPubSubClient(char *ip, uint16_t port);
void ReleaseCrpcPubSubClient(PubSubClient* client);
int CrpcSub(PubSubClient* client, char* topic);
int CrpcUnSub(PubSubClient* client, char* topic);
int CrpcPubMessage(PubSubClient* client, char* topic, uint8_t* payload, uint32_t payload_len);
void CrpcPubSubOnMessageCB(PubSubClient *client, SubOnMessage callback);

#ifdef __cplusplus
}
#endif
#endif // PUBSUB_CLIENT_H