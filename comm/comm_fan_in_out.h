#ifndef COMM_FAN_IN_OUT_H
#define COMM_FAN_IN_OUT_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "abs_comm.h"

// 与注册中心接收主题一致
#define REGISTRY_ID "registry_recv" //服务注册中心订阅的主题
#define COLLECTOR_ID "collector" //mstracing使用的主题
#define NODE_CENTER "node_center_recv" //节点中心订阅的主题

#ifdef __cplusplus
}
#endif
#endif // COMM_FAN_IN_OUT_H