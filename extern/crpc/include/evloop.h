#ifndef CRPC_EVENT_LOOP_H
#define CRPC_EVENT_LOOP_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(USE_SELECT) && !defined(USE_EPOLL)
#error "Either USE_SELECT or USE_EPOLL must be defined"
#endif

#if defined(USE_SELECT) && defined(USE_EPOLL)
#error "Only one of USE_SELECT or USE_EPOLL can be defined"
#endif

typedef struct FdMask FdMask;
struct FdMask {
    int fd;
    unsigned int mask;
};

typedef struct EventLoop EventLoop;
struct EventLoop {
    int maxFd;
    int setSize;
    FdMask *fdMasks;
    int stop;
#ifdef USE_SELECT
    fd_set readfds;
    fd_set writefds;
    fd_set exceptfds;
#endif
#ifdef USE_EPOLL
    int epfd;
    struct epoll_event *epEvents;
#endif
};

EventLoop *CreateEventLoop(int size);
void DestroyEventLoop(EventLoop *loop);
void StopEventLoop(EventLoop *loop);
int AddFdEvent(EventLoop *loop, int fd, unsigned int addMask);
int DelFdEvent(EventLoop *loop, int fd, unsigned int delMask);

#ifdef __cplusplus
}
#endif
#endif