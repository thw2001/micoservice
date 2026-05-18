#ifndef SHM_TEST_H
#define SHM_TEST_H
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "shm_comm.h"
#include <sys/shm.h>
#include <sys/time.h>

#define SHM_KEY 1236

#define SHM_SSSLEEP usleep(10000)

static int config_mem(){
    // shm_comm_config_add_node("registry", 1, 16384+1024, 100);
    // shm_comm_config_add_node("shm_lb_recv", 2, 16384+1024, 100);
    // shm_comm_config_add_node("shm_lb_send", 3, 16384+1024, 100);

    // int mem = shm_comm_cal_mem();
    // printf("use memory size: %d B\n", mem);
    // return mem;
}

// 获取当前时间戳（微秒）
static uint64_t get_timestamp_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000ULL + tv.tv_usec;
}

// 获取当前时间（纳秒）
static inline uint64_t get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

#if defined(__LINUX__)
static void init_shm1()
{
    int mem = config_mem();
    // 创建共享内存
    int shmid = shmget(SHM_KEY, mem, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(1);
    }

    // 附加共享内存
    void* shm_addr = shmat(shmid, NULL, 0);
    if (shm_addr == (void*)-1) {
        perror("shmat failed");
        exit(1);
    }

    shm_comm_init_minor(shm_addr);
}
#endif

#ifdef __cplusplus
}
#endif
#endif // SHM_TEST_H