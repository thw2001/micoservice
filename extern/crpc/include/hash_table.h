#ifndef CRPC_HASH_TABLE_H
#define CRPC_HASH_TABLE_H

#include "common.h"
#include "context.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * HashTable
 * _______________
 * |             |
 * |   ListNode  |----> ListNode ----> ListNode
 * |_____________|
 * |             |
 * |   ListNode  |----> ListNode ----> ListNode
 * |_____________|
 * |             |
 * |   ListNode  |----> ListNode ----> ListNode
 * |_____________|
 * |             |
 * |   ListNode  |----> ListNode ----> ListNode
 * |_____________|
 */

typedef struct ListNode ListNode;
struct ListNode {
    struct Context *context;
    struct ListNode *next;
};

typedef struct HashTable HashTable;
struct HashTable {
    unsigned int slots;
    int size;
    ListNode **list;
};

HashTable *InitHashTable(int slots);
void DestroyHashTable(HashTable *p);
Context *FindContext(HashTable *p, int fd);
int InsertHashTable(HashTable *p, Context *context);
void DeleteHashTable(HashTable *p, const Context *context);

#ifdef __cplusplus
}
#endif
#endif