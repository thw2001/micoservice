#include "context.h"

Context *CreateContext(int capacity)
{
    if (capacity < CONTEXT_BUFFER_MIN_SIZE) {
        capacity = CONTEXT_BUFFER_MIN_SIZE;
    } else if (capacity > CONTEXT_BUFFER_MAX_SIZE) {
        capacity = CONTEXT_BUFFER_MAX_SIZE;
    }
    Context *context = (Context *)calloc(1, sizeof(Context));
    if (context == NULL) {
        return NULL;
    }
    context->rCapacity = capacity;
    context->wCapacity = capacity;
    context->szRead = (char *)calloc(capacity, sizeof(char));
    if (context->szRead == NULL) {
        free(context);
        return NULL;
    }
    context->szWrite = (char *)calloc(capacity, sizeof(char));
    if (context->szWrite == NULL) {
        free(context->szRead);
        free(context);
        return NULL;
    }
    context->cSplit = '\t';
    context->cMsgEnd = "$$$$$$";
#if defined(WITH_PUB_SUB)
    map_init(&context->mapSubers);
#endif
    return context;
}
#if defined(WITH_PUB_SUB)
void DelMapSubers(Context* context)
{
    const char *key;
    map_iter_t iter = map_iter(&context->mapSubers);

    while ((key = map_next(&context->mapSubers, &iter)))
    {
        Subers* sub = map_get(&context->mapSubers, key);
        OM_TOPIC_LOCK(sub->om_topic);
        om_msg_del_suber(sub->suber);
        OM_TOPIC_UNLOCK(sub->om_topic);
    }
}
#endif
void ReleaseContext(Context *context)
{
    if (context == NULL) {
        return;
    }

    if (context != NULL) {
        free(context->szRead);
        free(context->szWrite);
#if defined(WITH_PUB_SUB)
        DelMapSubers(context);
        map_deinit(&context->mapSubers);
#endif
        free(context);
        context = NULL;
    }
}

/**
 * when context's read/write buffer is less then request len
 * expand the buffer size
 */
static int ExpandReadCache(Context *context, int len)
{
    if (context == NULL) {
        return -1;
    }

    int left = (context->rBegin <= context->rEnd) ? (context->rCapacity - 1 - context->rEnd + context->rBegin)
                                                  : (context->rBegin - context->rEnd - 1);
    if (left < len) {
        int capacity = context->rCapacity;
        while (left < len) {
            capacity += context->rCapacity;
            left += context->rCapacity;
        }
        char *p = (char *)calloc(capacity, sizeof(char));
        if (p == NULL) {
            return -1;
        }
        memmove(p, context->szRead, context->rCapacity);
        if (context->rBegin > context->rEnd) {
            memmove(p + context->rCapacity, p, context->rEnd);
        }
        char *pFree = context->szRead;
        context->szRead = p;
        if (context->rBegin > context->rEnd) {
            context->rEnd += context->rCapacity;
        }
        context->rCapacity = capacity;
        free(pFree);
    }
    return 0;
}

static int ExpandWriteCache(Context *context, int len)
{
    if (context == NULL) {
        return -1;
    }

    int left = (context->wBegin <= context->wEnd) ? (context->wCapacity - 1 - context->wEnd + context->wBegin)
                                                  : (context->wBegin - context->wEnd - 1);
    if (left < len) {
        int capacity = context->wCapacity;
        while (left < len) {
            capacity += context->wCapacity;
            left += context->wCapacity;
        }
        char *p = (char *)calloc(capacity, sizeof(char));
        if (p == NULL) {
            return -1;
        }
        memmove(p, context->szWrite, context->wCapacity);
        if (context->wBegin > context->wEnd) {
            memmove(p + context->wCapacity, p, context->wEnd);
        }
        char *pFree = context->szWrite;
        context->szWrite = p;
        if (context->wBegin > context->wEnd) {
            context->wEnd += context->wCapacity;
        }
        context->wCapacity = capacity;
        free(pFree);
    }
    return 0;
}

static int ContextAppendRead(Context *context, const char *buf, int len)
{
    if (context == NULL) {
        return -1;
    }

    if (ExpandReadCache(context, len) < 0) {
        return -1;
    }
    if (context->rEnd + len < context->rCapacity) {
        memmove(context->szRead + context->rEnd, buf, len);
        context->rEnd += len;
    } else {
        int tmp = context->rCapacity - context->rEnd;
        if (tmp > 0) {
            memmove(context->szRead + context->rEnd, buf, tmp);
        }
        if (tmp < len) {
            memmove(context->szRead, buf + tmp, len - tmp);
        }
        context->rEnd = len - tmp;
    }
    return 0;
}

int ContextAppendWrite(Context *context, const char *buf, int len)
{
    if (context == NULL) {
        return -1;
    }

    if (ExpandWriteCache(context, len) < 0) {
        return -1;
    }
    if (context->wEnd + len < context->wCapacity) {
        memmove(context->szWrite + context->wEnd, buf, len);
        context->wEnd += len;
    } else {
        int tmp = context->wCapacity - context->wEnd;
        if (tmp > 0) {
            memmove(context->szWrite + context->wEnd, buf, tmp);
        }
        if (tmp < len) {
            memmove(context->szWrite, buf + tmp, len - tmp);
        }
        context->wEnd = len - tmp;
    }
    return 0;
}

char *ContextGetReadRecord(Context *context)
{
    if (context == NULL) {
        return NULL;
    }

    if (context->rBegin == context->rEnd) {
        return NULL;
    }
    int len = (context->rBegin <= context->rEnd) ? (context->rEnd - context->rBegin)
                                                 : (context->rCapacity - context->rBegin + context->rEnd);
    char *buf = (char *)calloc(len + 1, sizeof(char));
    if (buf == NULL) {
        return NULL;
    }
    if (context->rBegin < context->rEnd) {
        memmove(buf, context->szRead + context->rBegin, len);
    } else {
        int tmp = context->rCapacity - context->rBegin;
        if (tmp > 0) {
            memmove(buf, context->szRead + context->rBegin, tmp);
        }
        if (context->rEnd > 0) {
            memmove(buf + tmp, context->szRead, context->rEnd);
        }
    }
    buf[len] = 0;
    char *p = strstr(buf, context->cMsgEnd);
    if (p == NULL) {
        free(buf);
        return NULL;
    }
    *p = 0;
    int num = p - buf + strlen(context->cMsgEnd);
    context->rBegin += num;
    if (context->rBegin >= context->rCapacity) {
        context->rBegin -= context->rCapacity;
    }
    return buf;
}

int ContextReadNet(Context *context)
{
    if (context == NULL) {
        return -1;
    }

    char line[MAX_ONE_LINE_SIZE] = {0};
    int ret = MyRead(context->fd, line, sizeof(line) - 1);
    if (ret == SOCK_ERR) {
        return SOCK_ERR;
    }
    int len = strlen(line);
    if (len > 0 && ContextAppendRead(context, line, len) < 0) {
        return -1;
    }
    return ret;
}

int ContextWriteNet(Context *context)
{
    if (context == NULL) {
        return -1;
    }

    if (context->wBegin == context->wEnd) {
        return 0;
    }
    if (context->wBegin < context->wEnd) {
        int ret = MyWrite(context->fd, context->szWrite + context->wBegin, context->wEnd - context->wBegin);
        if (ret > 0) {
            context->wBegin += ret;
        }
        return ret;
    }
    int len = context->wCapacity - context->wBegin;
    int ret = MyWrite(context->fd, context->szWrite + context->wBegin, len);
    if (ret < 0) {
        return ret;
    }
    if (ret < len) {
        context->wBegin += ret;
        return ret;
    }
    context->wBegin = 0;
    ret = MyWrite(context->fd, context->szWrite, context->wEnd);
    if (ret > 0) {
        context->wBegin = ret;
    }
    return ret;
}
