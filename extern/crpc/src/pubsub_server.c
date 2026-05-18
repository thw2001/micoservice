#include "pubsub_server.h"
#include "serial.h"
#include "om.h"
#include "log.h"
#include "pubsub_base.h"

om_status_t PubSubBrokerCallback(om_msg_t* msg, void* arg)
{
    Context *context = (Context *) arg;
    LOGD("context: %d %s %d %s", context->fd, msg->topic->name, msg->size, (char*)msg->buff);
    int i = 0;
    // printf("bin:\n");
    // for (i = 0; i < msg->size; i++)
    // {
    //     printf("%02x ", ((uint8_t*)msg->buff)[i]);
    // }
    // printf("\nend\n");
    WriteBegin(context, 1);
    WriteInt(context, msg->size);
    WriteStr(context, msg->topic->name);
    WriteUStr(context, msg->buff, msg->size);
    WriteEnd(context);

    int ret = 0;
    while (context->wBegin != context->wEnd && ret >= 0) {
        ret = ContextWriteNet(context);
    }
    if (ret < 0) {
        return ret;
    }
    return 0;
}

int PubSubBrokerCMDSub(PubSubBroker* server, Context *context, om_topic_t *om_topic)
{
    int ret = PUB_SUB_FAILED;
    Subers *tmp_sub = map_get(&context->mapSubers, om_topic->name);
    if (tmp_sub == NULL)
    {
        om_suber_t *suber = om_config_suber(NULL, "dt", PubSubBrokerCallback, context, om_topic);
        if (suber == NULL)
        {
            return CREATE_SUBCRIBER_FAILED;
        }
        Subers sub1 = {.om_topic = om_topic,.suber = suber};
        map_set(&context->mapSubers, om_topic->name, sub1);
        ret = PUB_SUB_SUCCESS;
    }
    else
    {
        ret = TOPIC_HAVE_SUBED;
    }
    return ret;
}

int PubSubBrokerCMDPub(PubSubBroker* server, Context *context, om_topic_t *om_topic)
{
    int payload_len = -1, ret = PUB_SUB_FAILED;
    ret = ReadInt(context, &payload_len);
    if (ret < 0 || payload_len < 0)
    {
        return READ_PAYLOAD_LEN_FAILED;
    }

    uint8_t *payload = (uint8_t *)calloc(payload_len + 1, sizeof(uint8_t));
    if (payload == NULL)
    {
        return ALLOCATE_MEMORY_FAILED;
    }

    ret = ReadUStr(context, payload, payload_len + 1);
    if (ret < 0)
    {
        free(payload);
        return READ_PAYLOAD_FAILED;
    }

    if (ret > 0)
    {
        free(payload);
        return READ_PAYLOAD_BUF_TOOLONG_FAILED;
    }

    om_topic->buff_len = payload_len;
    om_config_topic(om_topic, "Y", (void*)payload);
    
    om_publish(om_topic, payload, payload_len, true, false);

    free(payload);
    return PUB_SUB_SUCCESS;
}

int PubSubBrokerCMDUnSub(PubSubBroker* server, Context *context, om_topic_t *om_topic)
{
    int ret = PUB_SUB_FAILED;
    Subers *tmp_sub = map_get(&context->mapSubers, om_topic->name);
    if (tmp_sub == NULL)
    {
        ret = TOPIC_NOT_SUBED;
    }
    else
    {
        OM_TOPIC_LOCK(tmp_sub->om_topic);
        om_msg_del_suber(tmp_sub->suber);
        OM_TOPIC_UNLOCK(tmp_sub->om_topic);
        map_remove(&context->mapSubers, tmp_sub->om_topic->name);
        ret = PUB_SUB_SUCCESS;
    }

    return ret;
}

int PubSubBrokerOnTransact(void *server, Context *context)
{
    int cmd = -1, ret = -1, flag = -1;
    ret = ReadInt(context, &cmd);
    if (ret < 0)
    {
        flag = READ_CMD_FAILED;
        goto end_ret;
    }
    
    char topic[PUBSUB_TOPIC_NAME_LEN] = {0};
    ret = ReadStr(context, topic, PUBSUB_TOPIC_NAME_LEN);
    if (ret < 0)
    {
        flag = READ_TOPIC_FAILED;
        goto end_ret;
    }
    if (ret > 0)
    {
        flag = TOPIC_NAME_LEN_TOOLONG_FAILED;
        goto end_ret;
    }

    om_topic_t *om_topic = om_core_find_topic(topic, 0);
    if (om_topic == NULL)
    {
        om_topic = om_config_topic(NULL, "a", topic);
        if (om_topic == NULL)
        {
            flag = CREATE_TOPIC_FAILED;
            goto end_ret;
        }
    }
    
    switch (cmd)
    {
    case PUBSUB_CMD_SUB:
        flag = PubSubBrokerCMDSub(server, context, om_topic);
        break;
    case PUBSUB_CMD_PUB:
        flag = PubSubBrokerCMDPub(server, context, om_topic);
        break;
    case PUBSUB_CMD_UNSUB:
        flag = PubSubBrokerCMDUnSub(server, context, om_topic);
        break;
    default:
        LOGE("unknow cmd: %d", cmd);
        flag = UKNOWNCMD_FAILED;
        goto end_ret;
        break;
    }
end_ret:
    WriteBegin(context, 0);
    WriteInt(context, flag);
    WriteEnd(context);
    return 0;
}

PubSubBroker* CreatePubSubBroker(const char *ip, uint16_t port)
{
    PubSubBroker* broker = malloc(sizeof(PubSubBroker));
    broker->server = CreateRpcServer(ip, port);
    if (broker->server)
    {
        broker->server->arg = broker;
        broker->server->serverOnTransact = PubSubBrokerOnTransact;
        broker->server->serverOnCallbackTransact = NULL;
        broker->server->serverEndCallbackTransact = NULL;
        return broker;
    }
    
    return NULL;
}

int RunPubSubBrokerLoop(PubSubBroker* broker)
{
    return RunRpcLoop(broker->server);
}

void ReleasePubSubBroker(PubSubBroker* broker)
{
    ReleaseRpcServer(broker->server);
    free(broker);
}
