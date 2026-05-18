#include "evloop.h"
#include "log.h"

#undef LOG_TAG
#define LOG_TAG "CRpcEventLoop"

#define ELOOP_MAX_FD_SIZE 1024

EventLoop *CreateEventLoop(int size)
{
    int flag = 0;
    EventLoop *evLoop = NULL;
    do {
        evLoop = (EventLoop *)calloc(1, sizeof(EventLoop));
        if (evLoop == NULL) {
            LOGE("Failed to calloc EventLoop struct!");
            return NULL;
        }
        evLoop->setSize = size;
#ifdef USE_EPOLL
        evLoop->epfd = -1;
#endif
        if (size <= 0) {
            free(evLoop);
            return NULL;
        }
        evLoop->fdMasks = (FdMask *)calloc(size, sizeof(FdMask));
#ifdef USE_SELECT
        FD_ZERO(&evLoop->readfds);
        FD_ZERO(&evLoop->writefds);
        FD_ZERO(&evLoop->exceptfds);
#endif
#ifdef USE_EPOLL
        evLoop->epEvents = (struct epoll_event *)calloc(size, sizeof(struct epoll_event));
        if (evLoop->fdMasks == NULL || evLoop->epEvents == NULL) {
            flag = 1; /* fail */
            LOGE("Failed to calloc events or epoll_event struct!");
            break;
        }
        evLoop->epfd = epoll_create(ELOOP_MAX_FD_SIZE);
        if (evLoop->epfd == -1) {
            flag = 1; /* fail */
            LOGE("Failed to call epoll_create!");
            break;
        }
#endif
    } while (0);
    if (flag == 0) {
        return evLoop;
    }
    if (evLoop->fdMasks != NULL) {
        free(evLoop->fdMasks);
    }
#ifdef USE_EPOLL
    if (evLoop->epEvents != NULL) {
        free(evLoop->epEvents);
    }
#endif
    free(evLoop);
    return NULL;
}

void DestroyEventLoop(EventLoop *loop)
{
    if (loop == NULL) {
        return;
    }

#ifdef USE_EPOLL
    if (loop->epfd != -1) {
        close(loop->epfd);
    }
#endif
    if (loop->fdMasks != NULL) {
        free(loop->fdMasks);
    }
#ifdef USE_EPOLL
    if (loop->epEvents != NULL) {
        free(loop->epEvents);
    }
#endif
    free(loop);
    return;
}

void StopEventLoop(EventLoop *loop)
{
    if (loop == NULL) {
        return;
    }

    loop->stop = 1;
    return;
}

int AddFdEvent(EventLoop *loop, int fd, unsigned int addMask)
{
    if (loop == NULL) {
        return -1;
    }

    if (fd >= loop->setSize) {
        return -1;
    }
    if (loop->fdMasks[fd].mask & addMask) {
        return 0;
    }
#ifdef USE_SELECT
    if (addMask & READ_EVENT) {
        FD_SET(fd, &loop->readfds);
    }
    if (addMask & WRIT_EVENT) {
        FD_SET(fd, &loop->writefds);
    }
#endif
#ifdef USE_EPOLL
    int op = (loop->fdMasks[fd].mask == NONE_EVENT) ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
    addMask |= loop->fdMasks[fd].mask;
    struct epoll_event pollEvent = {0};
    if (addMask & READ_EVENT) {
        pollEvent.events |= EPOLLIN;
    }
    if (addMask & WRIT_EVENT) {
        pollEvent.events |= EPOLLOUT;
    }
    pollEvent.data.fd = fd;
    if (epoll_ctl(loop->epfd, op, fd, &pollEvent) == -1) {
        return -1;
    }
#endif
    loop->fdMasks[fd].fd = fd;
    loop->fdMasks[fd].mask |= addMask;
    if (fd > loop->maxFd) {
        loop->maxFd = fd;
    }
    return 0;
}

int DelFdEvent(EventLoop *loop, int fd, unsigned int delMask)
{
    if (loop == NULL) {
        return -1;
    }

    if (fd >= loop->setSize) {
        return 0;
    }
    if (loop->fdMasks[fd].mask == NONE_EVENT) {
        return 0;
    }
    if ((loop->fdMasks[fd].mask & delMask) == 0) {
        return 0;
    }
    unsigned int mask = loop->fdMasks[fd].mask & (~delMask);
#ifdef USE_SELECT
    if (delMask & READ_EVENT) {
        FD_CLR(fd, &loop->readfds);
    }
    if (delMask & WRIT_EVENT) {
        FD_CLR(fd, &loop->writefds);
    }
#endif
#ifdef USE_EPOLL
    struct epoll_event pollEvent = {0};
    pollEvent.events = 0;
    if (mask & READ_EVENT) {
        pollEvent.events |= EPOLLIN;
    }
    if (mask & WRIT_EVENT) {
        pollEvent.events |= EPOLLOUT;
    }
    pollEvent.data.fd = fd;
    int op = (mask == NONE_EVENT) ? EPOLL_CTL_DEL : EPOLL_CTL_MOD;
    if (epoll_ctl(loop->epfd, op, fd, &pollEvent) == -1) {
        return -1;
    }
#endif
    loop->fdMasks[fd].mask &= ~delMask;
    if (fd == loop->maxFd && loop->fdMasks[fd].mask == NONE_EVENT) {
        int j = loop->maxFd - 1;
        for (; j >= 0; --j) {
            if (loop->fdMasks[j].mask != NONE_EVENT) {
                break;
            }
        }
        loop->maxFd = j;
    }

    return 0;
}