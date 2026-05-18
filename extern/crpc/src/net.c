#include "net.h"
#include "log.h"
#undef LOG_TAG
#define LOG_TAG "CRpcNet"

int SetNonBlock(int fd, int type)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        LOGE("get socket flags failed!");
        return -1;
    }
    if (type == 1) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    if (fcntl(fd, F_SETFL, flags) < 0) {
        LOGE("set socket O_NONBLOCK failed!");
        return -1;
    }
    return 0;
}

int MyRead(int fd, char *buf, int count)
{
    int pos = 0;
    while (count > 0) {
        int ret = recv(fd, buf + pos, count, MSG_WAITALL);
        if (ret == 0) {
            buf[pos] = 0;
            return SOCK_CLOSE;
        } else if (ret < 0) {
            if (errno == EWOULDBLOCK || errno == EINTR || errno == EAGAIN) {
                break;
            } else {
                LOGE("read failed! error is %d", errno);
                return SOCK_ERR;
            }
        } else {
            pos += ret;
            count -= ret;
        }
    }
    buf[pos] = 0;
#ifdef DEBUG
    LOGD("read: %s", buf);
#endif
    return pos;
}

int MyWrite(int fd, const char *buf, int count)
{
    int pos = 0;
    while (count > 0) {
        int ret = send(fd, buf + pos, count, MSG_NOSIGNAL);
        if (ret == 0) {
            return SOCK_CLOSE;
        } else if (ret < 0) {
            if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) {
                break;
            } else {
                LOGE("write failed! error is %d", errno);
                return SOCK_ERR;
            }
        }
#ifdef DEBUG
        int tmpSize = ret + 1;
        char *szTmp = (char *)calloc(tmpSize, sizeof(char));
        if (szTmp != NULL) {
            strcpy(szTmp, buf + pos, ret);
            LOGD("write: %s", szTmp);
            free(szTmp);
        }
#endif
        pos += ret;
        count -= ret;
    }
    return pos;
}

static int CreateSocket(int domain)
{
    int sock = socket(domain, SOCK_STREAM, 0);
    if (sock < 0) {
        LOGE("create socket failed!");
        return -1;
    }
    return sock;
}

int CreateUnixServer(const char *ip, uint16_t port, int backlog)
{
    // struct sockaddr_un sockAddr;
    // memset(&sockAddr, 0, sizeof(sockAddr));
    // sockAddr.sun_family = AF_LOCAL;
    // strncpy(sockAddr.sun_path, path, sizeof(sockAddr.sun_path) - 1);
    int sock = CreateSocket(AF_INET);
    if (sock < 0) {
        return -1;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    // address.sin_addr.s_addr = INADDR_ANY;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int keepAlive = 1;
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive));
    int reuseaddr = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *)&reuseaddr, sizeof(reuseaddr));
    if (bind(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
        LOGE("bind failed!%d", errno);
        close(sock);
        return -1;
    }
    if (SetNonBlock(sock, 1) != 0) {
        LOGE("set socket non block failed!");
        close(sock);
        return -1;
    }
    fcntl(sock, F_SETFD, FD_CLOEXEC);
    if (listen(sock, backlog) < 0) {
        LOGE("listen failed!");
        close(sock);
        return -1;
    }
    return sock;
}

int ConnectUnixServer(const char *ip, uint16_t port)
{
    // struct sockaddr_un sockAddr;
    // memset(&sockAddr, 0, sizeof(sockAddr));
    // sockAddr.sun_family = AF_LOCAL;
    // strncpy(sockAddr.sun_path, path, sizeof(sockAddr.sun_path) - 1);
    int sock = CreateSocket(AF_INET);
    if (sock < 0) {
        return -1;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);

    if (connect(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
        LOGE("connect failed!");
        close(sock);
        return -1;
    }
    return sock;
}

int WaitFdEvent(int fd, unsigned int mask, int milliseconds)
{
    struct pollfd pFd = {0};
    pFd.fd = fd;
    if (mask & READ_EVENT) {
        pFd.events |= POLLIN;
    }
    if (mask & WRIT_EVENT) {
        pFd.events |= POLLOUT;
    }
    int ret = poll(&pFd, 1, milliseconds);
    if (ret < 0) {
        LOGE("poll failed!");
        return -1;
    } else if (ret == 0) {
        return 0;
    } else {
        return 1;
    }
}
