#ifndef PUBSUB_SERVER_H
#define PUBSUB_SERVER_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "server.h"

typedef struct PubSubBroker
{
    /* data */
    RpcServer* server;
} PubSubBroker;

PubSubBroker* CreatePubSubBroker(const char *ip, uint16_t port);
int RunPubSubBrokerLoop(PubSubBroker* broker);
void ReleasePubSubBroker(PubSubBroker* broker);

#ifdef __cplusplus
}
#endif
#endif // PUBSUB_SERVER_H