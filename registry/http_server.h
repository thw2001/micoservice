#ifndef _HTTP_SERVER_H
#define _HTTP_SERVER_H
#ifdef __cplusplus
extern "C"
{
#endif

#if defined(WITH_HTTP_SERVER)
#include "registry.h"
#include "node_center.h"

void registry_http_service_thread(registry* r, node_center* n);

#endif

#ifdef __cplusplus
}
#endif
#endif
