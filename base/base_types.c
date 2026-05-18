#include "base_types.h"
#include <sys/time.h>
#include "stdlib.h"
#include "stdio.h"

/**
 * 检查是否超时
 * @param time 前一时间
 * @param sec 预期超时时间，值为0时，表示获取当前时间
 * @return 0 - 未超时，1-超时
 */
int check_timeout(int* time, int sec)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int s = tv.tv_sec;
    int curr = tv.tv_sec;
    if (sec == 0)
    { // 0，取当前时间
        *time = curr;
        return 0;
    }
    else if (curr - *time >= sec)
    {                 // 非0检查超时
        // *time = curr; // 当超时时，才更新时间
        return 1;
    }
    return 0;
}
