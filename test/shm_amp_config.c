#include <signal.h> // 添加信号处理头文件
#include "pubsub.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "shm_comm.h"
#include <sys/shm.h>
#include <sys/time.h>
#define SHM_KEY 1236

int shmid = -1;
void* shm_addr = NULL;
pubsub_config_new_t pcfg = { 0 };

// 自定义信号处理函数
void sigint_handler(int signum) {
    printf("Caught SIGINT (Ctrl+C). Handling shm clear...\n");
    // 清理共享内存
    pubsub_config_new_deinit(&pcfg);
    shmdt(shm_addr);
    shmctl(shmid, IPC_RMID, NULL);
    printf("Shm clear complete\n");
}

int main(int argc, char const *argv[])
{
    const char* dev_files[] = {
        "microservice-config.json",
        "node-config.json",
    };
    pubsub_config_new_init(&pcfg);

    pub_init_config_files(&pcfg, dev_files, 2);
    int mem = pubsub_cal_mem(&pcfg);
    // 注册信号处理函数
    signal(SIGINT, sigint_handler);

    // 创建共享内存
    shmid = shmget(SHM_KEY, mem, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(1);
    }

    // 附加共享内存
    shm_addr = shmat(shmid, NULL, 0);
    if (shm_addr == (void*)-1) {
        perror("shmat failed");
        exit(1);
    }

    // shm_comm_clear();
    // shm_comm_init_main(shm_addr, mem);
    
    int ret = pubsub_init(&pcfg, 1, shm_addr, mem);
    

    printf("ret: %d\n", ret);
    // 等待信号
    pause();
    return 0;
}
