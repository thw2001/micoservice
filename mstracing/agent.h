#ifndef AGENT_H
#define AGENT_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "abs_comm.h"

typedef struct agent
{
    abs_comm* rc;
}agent;

agent* agent_new(abs_comm_connect* rct);
int agent_sendto_collector(agent* a, void* msg, uint32_t len);
void agent_delete(agent* a);

#ifdef __cplusplus
}
#endif
#endif // AGENT_H