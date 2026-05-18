#include "comm_fan_in_out.h"
#include "agent.h"
#include "stdio.h"
#include "stdlib.h"
#include "stddef.h"

agent *agent_new(abs_comm_connect* rct)
{
    agent* a = malloc(sizeof(agent));

    a->rc = abs_comm_new(NULL, rct);
    return a;
}

int agent_sendto_collector(agent *a, void *msg, uint32_t len)
{
    return a->rc->pub_msg(a->rc, COLLECTOR_ID, msg, len);
}

void agent_delete(agent *a)
{
    abs_comm_delete(a->rc);
    free(a);
    a = NULL;
}
