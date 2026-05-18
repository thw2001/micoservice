#ifndef _HTTP_SERVER_H
#define _HTTP_SERVER_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "collector.h"

void collector_http_service_thread(collector* c);

#ifdef __cplusplus
}
#endif
#endif
