#ifndef CPRC_LOG_H
#define CPRC_LOG_H

#include "stdio.h"
#include "time.h"

#ifndef LOG_TAG
#define LOG_TAG "CRPC"
#endif

#define LOG_DEBUG "Debug"
#define LOG_INFO "Info"
#define LOG_WARN "Warn"
#define LOG_ERROR "Error"

#define LOGPRINT(LOG_TAG, LOG_LEVEL, ...) \
    do {\
        time_t curr;\
        time(&curr);\
        struct tm cur_tm;\
        localtime_r(&curr, &cur_tm);\
        char cur_time[20];\
        snprintf(cur_time, sizeof(cur_time), "%d-%02d-%02d %02d:%02d:%02d",\
            cur_tm.tm_year + 1900, cur_tm.tm_mon + 1, cur_tm.tm_mday, cur_tm.tm_hour,\
            cur_tm.tm_min, cur_tm.tm_sec);\
        printf("%s %s %d %s %s ", cur_time, __FILE__, __LINE__, LOG_TAG, LOG_LEVEL);\
        printf(__VA_ARGS__);\
        printf("\n");\
    } while(0)

#define LOGD(...) LOGPRINT(LOG_TAG, LOG_DEBUG, ##__VA_ARGS__)
#define LOGI(...) LOGPRINT(LOG_TAG, LOG_INFO, ##__VA_ARGS__)
#define LOGW(...) LOGPRINT(LOG_TAG, LOG_WARN, ##__VA_ARGS__)
#define LOGE(...) LOGPRINT(LOG_TAG, LOG_ERROR, ##__VA_ARGS__)

#endif