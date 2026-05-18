#include "server.h"
#include "log.h"

#undef LOG_TAG
#define LOG_TAG "CRpcServer"

const int DEFAULT_LISTEN_QUEUE_SIZE = 10;
const int MAX_SUPPORT_CLIENT_FD_SIZE = 256; /* support max clients online */
const int DEFAULT_HASHTABLE_SLOTS = 7;
const int SERIAL_DATA_HEAD_SIZE = 2; /* RPC message head size: N| / C| just 2 */

static int BeforeLoop(RpcServer *server);
static int RemoveCallback(RpcServer *server, const Context *context);

static int OnAccept(RpcServer *server, unsigned int mask)
{
    if (server == NULL) {
        return -1;
    }

    if ((mask & READ_EVENT) == 0) {
        return 0;
    }
    int fd = accept(server->listenFd, NULL, NULL);
    if (fd < 0) {
        return -1;
    }
    SetNonBlock(fd, 1);
    fcntl(fd, F_SETFD, FD_CLOEXEC);
    Context *context = CreateContext(CONTEXT_BUFFER_MIN_SIZE);
    if (context != NULL) {
        context->fd = fd;
        InsertHashTable(server->clients, context);
        AddFdEvent(server->loop, fd, READ_EVENT | WRIT_EVENT);
    } else {
        close(fd);
        LOGE("Init Client context failed!");
        return -1;
    }
    return 0;
}

RpcServer *CreateRpcServer(const char *ip, uint16_t port)
{
    if (ip == NULL) {
        return NULL;
    }
    RpcServer *server = (RpcServer *)calloc(1, sizeof(RpcServer));
    server->arg = server;
    if (server == NULL) {
        return NULL;
    }
    int flag = 1;
    do {
        int ret = CreateUnixServer(ip, port, DEFAULT_LISTEN_QUEUE_SIZE);
        if (ret < 0) {
            break;
        }
        server->listenFd = ret;
        server->loop = CreateEventLoop(MAX_SUPPORT_CLIENT_FD_SIZE);
        if (server->loop == NULL) {
            break;
        }
        server->clients = InitHashTable(DEFAULT_HASHTABLE_SLOTS);
        if (server->clients == NULL) {
            break;
        }
        if (AddFdEvent(server->loop, server->listenFd, READ_EVENT) < 0) {
            break;
        }
        pthread_mutex_init(&server->mutex, NULL);
        flag = 0;
    } while (0);
    if (flag) {
        ReleaseRpcServer(server);
        return NULL;
    }
    return server;
}

static int DealReadMessage(RpcServer *server, Context *client)
{
    if ((server == NULL) || (client == NULL)) {
        return 0;
    }
    char *buf = ContextGetReadRecord(client);
    if (buf == NULL) {
        return 0;
    }
    client->oneProcess = buf;
    client->nPos = SERIAL_DATA_HEAD_SIZE; /* N| */
    client->nSize = strlen(buf);
    if (server->serverOnTransact != NULL)
    {
        server->serverOnTransact(server->arg, client);
    }
    free(buf);
    AddFdEvent(server->loop, client->fd, WRIT_EVENT);
    return 1;
}
#if defined(USE_EPOLL)
static unsigned int CheckEventMask(const struct epoll_event *e)
{
    if (e == NULL) {
        return 0;
    }
    unsigned int mask = NONE_EVENT;
    if ((e->events & EPOLLERR) || (e->events & EPOLLHUP)) {
        mask |= READ_EVENT | WRIT_EVENT | EXCP_EVENT;
    } else {
        if (e->events & EPOLLIN) {
            mask |= READ_EVENT;
        }
        if (e->events & EPOLLOUT) {
            mask |= WRIT_EVENT;
        }
    }
    return mask;
}
#endif

static void DealFdReadEvent(RpcServer *server, Context *client, unsigned int mask)
{
    if ((server == NULL) || (client == NULL)) {
        return;
    }
    DealReadMessage(server, client);
    int ret = ContextReadNet(client);
    if ((ret == SOCK_ERR) || ((ret == SOCK_CLOSE) && (mask & EXCP_EVENT))) {
        DelFdEvent(server->loop, client->fd, READ_EVENT | WRIT_EVENT);
    } else if (ret == SOCK_CLOSE) {
        DelFdEvent(server->loop, client->fd, READ_EVENT);
    } else if (ret > 0) {
        int haveMsg;
        do {
            haveMsg = DealReadMessage(server, client);
        } while (haveMsg);
    }
    return;
}

static void DealFdWriteEvent(RpcServer *server, Context *client)
{
    if ((server == NULL) || (client == NULL)) {
        return;
    }

    if (client->wBegin != client->wEnd) {
        int tmp = ContextWriteNet(client);
        if (tmp < 0) {
            DelFdEvent(server->loop, client->fd, READ_EVENT | WRIT_EVENT);
        }
    } else {
        DelFdEvent(server->loop, client->fd, WRIT_EVENT);
    }
    return;
}

static void DealFdEvents(RpcServer *server, int fd, unsigned int mask)
{
    if (server == NULL) {
        return;
    }
    Context *client = FindContext(server->clients, fd);
    if (client == NULL) {
        LOGD("not find %d clients!", fd);
        return;
    }
    if (mask & READ_EVENT) {
        DealFdReadEvent(server, client, mask);
    }
    if (mask & WRIT_EVENT) {
        DealFdWriteEvent(server, client);
    }
    if (server->loop->fdMasks[fd].mask == NONE_EVENT) {
        close(fd);
        DeleteHashTable(server->clients, client);
        RemoveCallback(server, client);
        ReleaseContext(client);
    }
    return;
}

int RunRpcLoop(RpcServer *server)
{
    if (server == NULL) {
        return -1;
    }

    EventLoop *loop = server->loop;
    while (!loop->stop) {
        BeforeLoop(server);
#ifdef USE_SELECT
        fd_set readfds = loop->readfds;
        fd_set writefds = loop->writefds;
        fd_set exceptfds = loop->exceptfds;
        struct timeval tv = {0, 1000}; // 1ms timeout
        int retval = select(loop->maxFd + 1, &readfds, &writefds, &exceptfds, &tv);
        if (retval > 0) {
            for (int fd = 0; fd <= loop->maxFd; ++fd) {
                unsigned int mask = NONE_EVENT;
                if (FD_ISSET(fd, &readfds)) mask |= READ_EVENT;
                if (FD_ISSET(fd, &writefds)) mask |= WRIT_EVENT;
                if (mask != NONE_EVENT) {
                    if (fd == server->listenFd) {
                        OnAccept(server, mask);
                    } else {
                        DealFdEvents(server, fd, mask);
                    }
                }
            }
        }
#endif
#ifdef USE_EPOLL
        int retval = epoll_wait(loop->epfd, loop->epEvents, loop->setSize, 1); /* wait 1ms */
        for (int i = 0; i < retval; ++i) {
            struct epoll_event *e = loop->epEvents + i;
            int fd = e->data.fd;
            unsigned int mask = CheckEventMask(e);
            if (fd == server->listenFd) {
                OnAccept(server, mask);
            } else {
                DealFdEvents(server, fd, mask);
            }
        }
#endif
    }
    return 0;
}

void ReleaseRpcServer(RpcServer *server)
{
    if (server != NULL) {
        if (server->clients != NULL) {
            DestroyHashTable(server->clients);
        }
        if (server->loop != NULL) {
            StopEventLoop(server->loop);
            DestroyEventLoop(server->loop);
        }
        if (server->listenFd > 0) {
            close(server->listenFd);
        }
        pthread_mutex_destroy(&server->mutex);
        free(server);
    }
}

static int BeforeLoop(RpcServer *server)
{
    if (server == NULL) {
        return -1;
    }
    pthread_mutex_lock(&server->mutex);
    for (int i = 0; i < server->nEvents; ++i) {
        int event = server->events[i];
        int num = sizeof(server->eventNode) / sizeof(server->eventNode[0]);
        int pos = event % num;
        struct Node *p = server->eventNode[pos].head;
        while (p != NULL) {
            Context *context = p->context;
            if (server->serverOnCallbackTransact != NULL)
            {
                server->serverOnCallbackTransact(server->arg, event, context);
            }
            AddFdEvent(server->loop, context->fd, WRIT_EVENT);
            p = p->next;
        }
        if (server->serverEndCallbackTransact != NULL)
        {
            server->serverEndCallbackTransact(server->arg, event);
        }
    }
    server->nEvents = 0;
    pthread_mutex_unlock(&server->mutex);
    return 0;
}

int EmitEvent(RpcServer *server, int event)
{
    if (server == NULL) {
        return -1;
    }
    int num = sizeof(server->events) / sizeof(server->events[0]);
    pthread_mutex_lock(&server->mutex);
    if (server->nEvents >= num) {
        pthread_mutex_unlock(&server->mutex);
        return -1;
    }
    server->events[server->nEvents] = event;
    ++server->nEvents;
    pthread_mutex_unlock(&server->mutex);
    return 0;
}

int RegisterCallback(RpcServer *server, int event, Context *context)
{
    if ((server == NULL) || (context == NULL)) {
        return -1;
    }

    int num = sizeof(server->eventNode) / sizeof(server->eventNode[0]);
    int pos = event % num;
    server->eventNode[pos].event = event;
    struct Node *p = server->eventNode[pos].head;
    while (p != NULL && p->context->fd != context->fd) {
        p = p->next;
    }
    if (p == NULL) {
        p = (struct Node *)calloc(1, sizeof(struct Node));
        if (p != NULL) {
            p->next = server->eventNode[pos].head;
            p->context = context;
            server->eventNode[pos].head = p;
        }
    }
    return 0;
}

int UnRegisterCallback(RpcServer *server, int event, const Context *context)
{
    if ((server == NULL) || (context == NULL)) {
        return -1;
    }

    int num = sizeof(server->eventNode) / sizeof(server->eventNode[0]);
    int pos = event % num;
    server->eventNode[pos].event = event;
    struct Node *p = server->eventNode[pos].head;
    struct Node *q = p;
    while (p != NULL && p->context->fd != context->fd) {
        q = p;
        p = p->next;
    }
    if (p != NULL) {
        if (p == server->eventNode[pos].head) {
            server->eventNode[pos].head = p->next;
        } else {
            q->next = p->next;
        }
        free(p);
    }
    return 0;
}

static int RemoveCallback(RpcServer *server, const Context *context)
{
    if ((server == NULL) || (context == NULL)) {
        return -1;
    }

    int num = sizeof(server->eventNode) / sizeof(server->eventNode[0]);
    for (int i = 0; i < num; ++i) {
        struct Node *p = server->eventNode[i].head;
        if (p == NULL) {
            continue;
        }
        struct Node *q = p;
        while (p != NULL && p->context->fd != context->fd) {
            q = p;
            p = p->next;
        }
        if (p != NULL) {
            if (p == server->eventNode[i].head) {
                server->eventNode[i].head = p->next;
            } else {
                q->next = p->next;
            }
            free(p);
        }
    }
    return 0;
}

void SetCallbackServerOnTransact(RpcServer *server, ServerOnTransact cb)
{
    server->serverOnTransact = cb;
}

void SetCallbackServerOnCallbackTransact(RpcServer *server, ServerOnCallbackTransact cb)
{
    server->serverOnCallbackTransact = cb;
}

void SetCallbackServerEndCallbackTransact(RpcServer *server, ServerEndCallbackTransact cb)
{
    server->serverEndCallbackTransact = cb;
}

#define RPC_REGISTER_MAX_NUM 256

int RpcRegisterEventCallback(RpcServer *server, Context *context)
{
    if (server == NULL || context == NULL) {
        return CRPC_FAILED;
    }
    int num = 0;
    if (ReadInt(context, &num) < 0) {
        return CRPC_FAILED;
    }
    if (num < 0 || num > RPC_REGISTER_MAX_NUM) {
        return CRPC_FAILED;
    }
    int *events = ReadIntArray(context, num);
    if (events == NULL) {
        return CRPC_FAILED;
    }
    for (int i = 0; i < num; ++i) {
        RegisterCallback(server, events[i], context);
    }
    WriteBegin(context, 0);
    WriteInt(context, CRPC_SUCCESS);
    WriteEnd(context);
    free(events);
    events = NULL;
    return CRPC_SUCCESS;
}

int RpcUnRegisterEventCallback(RpcServer *server, Context *context)
{
    if (server == NULL || context == NULL) {
        return CRPC_FAILED;
    }
    int num = 0;
    if (ReadInt(context, &num) < 0) {
        return CRPC_FAILED;
    }
    if (num < 0 || num > RPC_REGISTER_MAX_NUM) {
        return CRPC_FAILED;
    }
    int *events = ReadIntArray(context, num);
    if (events == NULL) {
        return CRPC_FAILED;
    }
    for (int i = 0; i < num; ++i) {
        UnRegisterCallback(server, events[i], context);
    }
    WriteBegin(context, 0);
    WriteInt(context, CRPC_SUCCESS);
    WriteEnd(context);
    free(events);
    events = NULL;
    return CRPC_SUCCESS;
}