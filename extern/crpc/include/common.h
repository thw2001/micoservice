#ifndef CRPC_COMMON_H
#define CRPC_COMMON_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <poll.h>
#include <arpa/inet.h>
#if defined(USE_SELECT)
#include <sys/select.h>
#elif defined(USE_EPOLL)
#include <sys/epoll.h>
#endif
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CRPC_SUCCESS    0
#define CRPC_FAILED -1

// 定义错误码
#define SOCK_CLOSE (-200)
#define SOCK_ERR (-100)

// 定义事件
#define NONE_EVENT (0x00)
#define READ_EVENT (0x01)
#define WRIT_EVENT (0x02)
#define EXCP_EVENT (0x04)

// 其他定义
#define INSERT_HASH_DUPLICATE (-2)
#define SERIAL_READ_TOO_LONG (-2)
#define SERIAL_READ_INVALID_DIGIT (-3)
#define MAX_ONE_LINE_SIZE (1024)

#ifdef __cplusplus
}
#endif
#endif