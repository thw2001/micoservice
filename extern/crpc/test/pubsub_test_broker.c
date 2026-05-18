#include "pubsub_server.h"

int main(int argc, char const *argv[])
{
    PubSubBroker* server = CreatePubSubBroker("127.0.0.1", 19999);
    RunPubSubBrokerLoop(server);
    ReleasePubSubBroker(server);
    return 0;
}

