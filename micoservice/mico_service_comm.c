#include "mico_service.h"
#include "base_types.h"
#include "cf_log.h"

int mico_service_sub(mico_service* ms, 
                    char* topic, void *arg, 
                    sub_topic_callback recv_cb)
{
    int ret = abs_comm_sub(ms->rc, topic, arg, recv_cb);

    return ret;
}

int mico_service_pub(mico_service* ms, char* topic, void* msg, uint32_t len)
{
    if (NULL == ms || NULL == ms->rc)
    {
        return BASE_FAILED;
    }
    
    return ms->rc->pub_msg(ms->rc, topic, msg, len);
}