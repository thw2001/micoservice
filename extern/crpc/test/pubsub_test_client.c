#include "pubsub_client.h"
#include <stdio.h>
#include <time.h>

int thistime() {
    time_t rawtime;
    char *time_str;

    time(&rawtime);
    time_str = ctime(&rawtime);

    printf("Current date and time: %s", time_str);

    return 0;
}

int testCB(void *arg, char* topic, uint8_t* payload, uint32_t len)
{
    thistime();
    printf("topic: %s, len: %d\n", topic, len);
    return 0;
}

int testCB2(void *arg, char* topic, uint8_t* payload, uint32_t len)
{
    thistime();
    printf("topic2: %s, len: %d\n", topic, len);
    return 0;
}

#define size 10000
uint8_t a[size];

int main(int argc, char const *argv[])
{
    PubSubClient* client = CreateCrpcPubSubClient("127.0.0.1", 19999);
    CrpcSub(client, "test_topic1");
    CrpcPubSubOnMessageCB(client, testCB);

    PubSubClient* client2 = CreateCrpcPubSubClient("127.0.0.1", 19999);
    CrpcSub(client2, "ms1");
    CrpcPubSubOnMessageCB(client2, testCB2);
    CrpcPubMessage(client, "test_topic1", "bbb\0b\t", 7);
    CrpcPubMessage(client2, "ms1", "bbbb\0b\t", 8);
    
    memset(a, '\t', size-1);
    a[size-1] = '\0';

    while (1)
    {
        // CrpcSub(client, "test_topic1");
        thistime();
        // CrpcPubMessage(client, "test_topic1", a, size);
        // CrpcUnSub(client, "test_topic1");
        CrpcPubMessage(client, "ms1", "bbb\0b\t", 7);
        // CrpcPubMessage(client2, "test_topic1", "bbbb\0b\t", 8);
        sleep(1);
    }
    
    return 0;
}
