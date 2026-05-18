#ifndef CRPC_NET_H
#define CRPC_NET_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

int SetNonBlock(int fd, int type);
int MyRead(int fd, char *buf, int count);
int MyWrite(int fd, const char *buf, int count);
int CreateUnixServer(const char *ip, uint16_t port, int backlog);
int ConnectUnixServer(const char *ip, uint16_t port);
int WaitFdEvent(int fd, unsigned int mask, int milliseconds);

#ifdef __cplusplus
}
#endif
#endif