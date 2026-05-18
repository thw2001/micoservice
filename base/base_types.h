#ifndef BASE_TYPES_H
#define BASE_TYPES_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifndef FS_BASE
#define FS_BASE "/c/" //reworks路径是/c/
#endif

#define BASE_SUCCESS    0
#define BASE_FAILED     -1


// #define MSG_TYPE_SERVICE_INFO   1
// #define MSG_TYPE_NODE_INFP  2
// #define MSG_TYPE_REGISTRY_TO_SERVICE   3
// #define MSG_TYPE_NODE_DEPLOY_CMD   4

enum {
    MSG_TYPE_SERVICE_INFO = 0,
    MSG_TYPE_NODE_INFP,
    MSG_TYPE_REGISTRY_TO_SERVICE,
    MSG_TYPE_NODE_DEPLOY_CMD,
    MSG_TYPE_FILE_TRANS_CMD
};

#define THREAD_RUNING 1
#define THREAD_STOPING 0

int check_timeout(int* time, int sec);

#ifdef __cplusplus
}
#endif
#endif // BASE_TYPES_H