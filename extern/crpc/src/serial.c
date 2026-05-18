#include <inttypes.h>
#include "serial.h"

#define TMP_CHAR_LEN 64

int WriteBegin(Context *context, int type)
{
    if (context == NULL) {
        return -1;
    }

    char szTmp[TMP_CHAR_LEN] = {0};
    int ret = snprintf(szTmp, sizeof(szTmp), "%s%c", ((type == 0) ? "N" : "C"), context->cSplit);
    if (ret < 0 || ret >= sizeof(szTmp)) {
        return -1;
    }
    int len = strlen(szTmp);
    return ContextAppendWrite(context, szTmp, len);
}

int WriteFunc(Context *context, const char *funcName)
{
    if (context == NULL || funcName == NULL) {
        return -1;
    }

    int len = strlen(funcName) + 1;
    if (len <= 0) {
        return -1;
    }
    char *buf = (char *)calloc(len + 1, sizeof(char));
    if (buf == NULL) {
        return -1;
    }
    int ret = snprintf(buf, len + 1, "%s%c", funcName, context->cSplit);
    if (ret < 0 || ret >= len + 1) {
        free(buf);
        return -1;
    }
    ret = ContextAppendWrite(context, buf, len);
    free(buf);
    return ret;
}

int WriteInt(Context *context, int iData)
{
    if (context == NULL) {
        return -1;
    }

    char szTmp[TMP_CHAR_LEN] = {0};
    int ret = snprintf(szTmp, sizeof(szTmp), "%d%c", iData, context->cSplit);
    if (ret < 0 || ret >= sizeof(szTmp)) {
        return -1;
    }
    return ContextAppendWrite(context, szTmp, strlen(szTmp));
}

int WriteLong(Context *context, long lData)
{
    if (context == NULL) {
        return -1;
    }

    char szTmp[TMP_CHAR_LEN] = {0};
    int ret = snprintf(szTmp, sizeof(szTmp), "%ld%c", lData, context->cSplit);
    if (ret < 0 || ret >= sizeof(szTmp)) {
        return -1;
    }
    return ContextAppendWrite(context, szTmp, strlen(szTmp));
}

int WriteInt64(Context *context, int64_t iData)
{
    if (context == NULL) {
        return -1;
    }

    char szTmp[TMP_CHAR_LEN] = {0};
    int ret = snprintf(szTmp, sizeof(szTmp), "%" PRIu64 "%c", iData, context->cSplit);
    if (ret < 0 || ret >= sizeof(szTmp)) {
        return -1;
    }
    return ContextAppendWrite(context, szTmp, strlen(szTmp));
}

int WriteDouble(Context *context, double dData)
{
    if (context == NULL) {
        return -1;
    }

    char szTmp[TMP_CHAR_LEN] = {0};
    int ret = snprintf(szTmp, sizeof(szTmp), "%.6lf%c", dData, context->cSplit);
    if (ret < 0 || ret >= sizeof(szTmp)) {
        return -1;
    }
    return ContextAppendWrite(context, szTmp, strlen(szTmp));
}

int WriteChar(Context *context, char cData)
{
    if (context == NULL) {
        return -1;
    }

    char szTmp[TMP_CHAR_LEN] = {0};
    int ret = snprintf(szTmp, sizeof(szTmp), "%c%c", cData, context->cSplit);
    if (ret < 0 || ret >= sizeof(szTmp)) {
        return -1;
    }
    return ContextAppendWrite(context, szTmp, strlen(szTmp));
}

int WriteStr(Context *context, const char *pStr)
{
    if (context == NULL || pStr == NULL) {
        return -1;
    }

    int len = strlen(pStr) + 1;
    if (len <= 0) {
        return -1;
    }
    char *buf = (char *)calloc(len + 1, sizeof(char));
    if (buf == NULL) {
        return -1;
    }
    int ret = snprintf(buf, len + 1, "%s%c", pStr, context->cSplit);
    if (ret < 0 || ret >= len + 1) {
        free(buf);
        return -1;
    }
    ret = ContextAppendWrite(context, buf, len);
    free(buf);
    return ret;
}

static const char hexDigits[] = "0123456789abcdef";

int WriteUStr(Context *context, const unsigned char *uStr, unsigned int len)
{
    if (context == NULL || uStr == NULL) {
        return -1;
    }

    int inLen = (len << 1) + 1;
    char *buf = (char *)calloc(inLen + 1, sizeof(char));
    if (buf == NULL) {
        return -1;
    }

    char *p = buf;
    for (unsigned int i = 0; i < len; ++i) {
        *p++ = hexDigits[(uStr[i] >> 4) & 0xF];
        *p++ = hexDigits[uStr[i] & 0xF];
    }
    *p++ = context->cSplit;
    *p = 0;

    int ret = ContextAppendWrite(context, buf, inLen);
    free(buf);
    return ret;
}

int WriteEnd(Context *context)
{
    if (context == NULL) {
        return -1;
    }

    return ContextAppendWrite(context, context->cMsgEnd, strlen(context->cMsgEnd));
}

static int ReadNext(Context *context)
{
    if (context == NULL) {
        return -1;
    }

    if (context->oneProcess == NULL || context->nPos >= context->nSize) {
        return -1;
    }
    char *p = context->oneProcess + context->nPos;
    char *q = p;
    while (*p != context->cSplit && *p != '\0') {
        ++p;
    }
    if (*p != context->cSplit) {
        return -1;
    }
    return p - q;
}

int ReadFunc(Context *context, char *funcName, int count)
{
    if (context == NULL) {
        return -1;
    }

    int len = ReadNext(context);
    if (len < 0) {
        return len;
    }
    if (len >= count) {
        return len;
    }
    strncpy(funcName, context->oneProcess + context->nPos, len);
    context->nPos += len + 1;
    return 0;
}

static int IsValidNumberChar(const char *szNum, int size)
{
    int dotNum = 0; /* '.' num */
    for (int i = 0; i < size; ++i) {
        if (szNum[i] == '.') {
            ++dotNum;
        } else if ((szNum[i] < '0' || szNum[i] > '9') && szNum[i] != '+' && szNum[i] != '-') {
            return -1;
        }
    }
    if (dotNum > 1) {
        return -1;
    }
    return 0;
}

static int ReadNextNum(Context *context, char *szNum, int size)
{
    if (context == NULL) {
        return -1;
    }

    int len = ReadNext(context);
    if (len < 0) {
        return len;
    }
    if (len >= size) {
        return SERIAL_READ_TOO_LONG;
    }
    strncpy(szNum, context->oneProcess + context->nPos, len);
    if (IsValidNumberChar(szNum, len) != 0) {
        return SERIAL_READ_INVALID_DIGIT;
    }
    return len;
}

int ReadInt(Context *context, int *iData)
{
    if (context == NULL) {
        return -1;
    }

    char szTmp[TMP_CHAR_LEN] = {0};
    int len = ReadNextNum(context, szTmp, sizeof(szTmp));
    if (len < 0) {
        return len;
    }
    *iData = atoi(szTmp);
    context->nPos += len + 1;
    return 0;
}

int ReadLong(Context *context, long *pLong)
{
    if (context == NULL) {
        return -1;
    }

    char szTmp[TMP_CHAR_LEN] = {0};
    int len = ReadNextNum(context, szTmp, sizeof(szTmp));
    if (len < 0) {
        return len;
    }
    *pLong = atol(szTmp);
    context->nPos += len + 1;
    return 0;
}

int ReadInt64(Context *context, int64_t *pInt64)
{
    if (context == NULL) {
        return -1;
    }

    char szTmp[TMP_CHAR_LEN] = {0};
    int len = ReadNextNum(context, szTmp, sizeof(szTmp));
    if (len < 0) {
        return len;
    }
    *pInt64 = atoll(szTmp);
    context->nPos += len + 1;
    return 0;
}

int ReadDouble(Context *context, double *dData)
{
    if (context == NULL) {
        return -1;
    }

    char szTmp[TMP_CHAR_LEN] = {0};
    int len = ReadNextNum(context, szTmp, sizeof(szTmp));
    if (len < 0) {
        return len;
    }
    *dData = atof(szTmp);
    context->nPos += len + 1;
    return 0;
}

int ReadChar(Context *context, char *cData)
{
    if (context == NULL) {
        return -1;
    }

    int len = ReadNext(context);
    if (len < 0) {
        return len;
    }
    if (len != 1) {
        return SERIAL_READ_TOO_LONG;
    }
    *cData = *(context->oneProcess + context->nPos);
    context->nPos += len + 1;
    return 0;
}

int ReadStr(Context *context, char *str, int count)
{
    if (context == NULL) {
        return -1;
    }

    int len = ReadNext(context);
    if (len < 0) {
        return len;
    }
    if (str == NULL || len >= count) {
        return len;
    }
    strncpy(str, context->oneProcess + context->nPos, len);
    context->nPos += len + 1;
    return 0;
}

static const unsigned char hexToByte[256] = {
    [0 ... 9] = 0,
    ['0'] = 0, ['1'] = 1, ['2'] = 2, ['3'] = 3, ['4'] = 4,
    ['5'] = 5, ['6'] = 6, ['7'] = 7, ['8'] = 8, ['9'] = 9,
    ['a'] = 10, ['b'] = 11, ['c'] = 12, ['d'] = 13, ['e'] = 14, ['f'] = 15
};

int ReadUStr(Context *context, unsigned char *uStr, int count)
{
    if (context == NULL) {
        return -1;
    }

    int len = ReadNext(context);
    if (len < 0) {
        return len;
    }
    if ((len & 1) != 0) { /* format invalid: uStr message len must be an even number */
        return -1;
    }
    if ((len >> 1) >= count) {
        return (len >> 1);
    }

    char *q = context->oneProcess + context->nPos;
    for (int i = 0; i < len; i += 2) {
        unsigned char byte = (hexToByte[q[i]] << 4) | hexToByte[q[i + 1]];
        uStr[i / 2] = byte;
    }
    uStr[len / 2] = 0;
    context->nPos += len + 1;
    return 0;
}

int *ReadIntArray(Context *context, int size)
{
    if (size <= 0) {
        return NULL;
    }

    int *pArray = (int *)calloc(size, sizeof(int));
    if (pArray == NULL) {
        return NULL;
    }
    for (int i = 0; i < size; ++i) {
        if (ReadInt(context, pArray + i) < 0) {
            free(pArray);
            pArray = NULL;
            return NULL;
        }
    }
    return pArray;
}

char **ReadCharArray(Context *context, int size)
{
    char **pArray = NULL;
    if (size > 0) {
        pArray = (char **)calloc(size, sizeof(char *));
    }
    if (pArray == NULL) {
        return NULL;
    }
    int i = 0;
    for (; i < size; ++i) {
        int len = 0;
        if (ReadInt(context, &len) < 0) {
            break;
        }
        pArray[i] = (char *)calloc(len + 1, sizeof(char));
        if (pArray[i] == NULL) {
            break;
        }
        if (ReadStr(context, pArray[i], len + 1) < 0) {
            break;
        }
    }
    if (i < size) {
        for (int j = 0; j < size; ++j) {
            free(pArray[j]);
            pArray[j] = NULL;
        }
        free(pArray);
        pArray = NULL;
        return NULL;
    }
    return pArray;
}
