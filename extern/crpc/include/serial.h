#ifndef CRPC_SERIAL_H
#define CRPC_SERIAL_H

#include <stdint.h>
#include "common.h"
#include "context.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BASE_MSG    0   /* (char {N}) */
#define CALLBACK_MSG    1   /* msg: server pub to client. client will de-serial this type(char {C}) */
int WriteBegin(Context *context, int type);
int WriteFunc(Context *context, const char *funcName);
int WriteInt(Context *context, int i);
int WriteLong(Context *context, long L);
int WriteInt64(Context *context, int64_t i);
int WriteDouble(Context *context, double d);
int WriteChar(Context *context, char c);
int WriteStr(Context *context, const char *str);
int WriteUStr(Context *context, const unsigned char *uStr, unsigned int len);
int WriteEnd(Context *context);

int ReadFunc(Context *context, char *funcName, int count);
int ReadInt(Context *context, int *i);
int ReadLong(Context *context, long *L);
int ReadInt64(Context *context, int64_t *i);
int ReadDouble(Context *context, double *d);
int ReadChar(Context *context, char *c);
int ReadStr(Context *context, char *str, int count);
int ReadUStr(Context *context, unsigned char *uStr, int count);

int *ReadIntArray(Context *context, int size);
char **ReadCharArray(Context *context, int size);

#ifdef __cplusplus
}
#endif
#endif