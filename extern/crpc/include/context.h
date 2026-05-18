#ifndef CRPC_CONTEXT_H
#define CRPC_CONTEXT_H

#include "common.h"
#include "net.h"
#if defined(WITH_PUB_SUB)
#include "om.h"
#include "map.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define CONTEXT_BUFFER_MIN_SIZE (1024)
#define CONTEXT_BUFFER_MAX_SIZE (1024000)

#if defined(WITH_PUB_SUB)
typedef struct Subers Subers;
struct Subers
{
    om_topic_t* om_topic;
    om_suber_t* suber;
};

typedef map_t(Subers) MapSubers;
#endif
/**
 * Context 通信上下文
 * RPC框架内部使用unix域套接字方式通信，一个套接字对应一个通信上下文
 * 结构中包含： 套接字、读缓存、写缓存、处理数据缓存
 * 缓存采用循环数组方式，begin标识下一次读写位置，end标识当前读写截止位置
 */
typedef struct Context Context;
struct Context {
    int fd;
    char *szRead; // 读缓存
    unsigned int rCapacity; // 缓存容量
    unsigned int rBegin; // 当前读取位置
    unsigned int rEnd; // 读缓存有效位置
    char *szWrite;
    unsigned int wCapacity;
    unsigned int wBegin;
    unsigned int wEnd;
    char *oneProcess; // 处理读数据时，拷贝到此处
    unsigned int nPos; // 处理读数据位置
    unsigned int nSize; // 处理数据的大小
    char cSplit; // 数据间截取标识
    const char *cMsgEnd; // 数据结束标识
#if defined(WITH_PUB_SUB)
    MapSubers mapSubers;
#endif
};

Context *CreateContext(int capacity);
void ReleaseContext(Context *context);
int ContextReadNet(Context *context);
int ContextWriteNet(Context *context);
int ContextAppendWrite(Context *context, const char *buf, int len);
char *ContextGetReadRecord(Context *context);

#ifdef __cplusplus
}
#endif
#endif