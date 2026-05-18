#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mico_service.h"
#include <unistd.h>

int sendto_t2_callback(void* arg, void* msg, uint32_t len) {
    mico_service* ms_ptr = arg;
    char* msg_data = (char*)msg;
    if (len > 0) {
        switch (msg_data[0]) {
            case 'A': // 延时测试消息
                mico_service_pub(ms_ptr, "recvfrom_t2", msg, len);
                break;
            case 'B': // 吞吐量测试消息
                // 不回复，仅接收
                break;
            default:
                printf("Unknown test type: %c\n", msg_data[0]);
        }
    }
    return 0;
}

int main(int argc, char const *argv[]) {
    int ret = 0;
    abs_comm_connect rct = {.ip = "127.0.0.1", .port = 19999};
    mico_service* ms = mico_service_info_new("test2.json", &rct);
    if (ms == NULL) {
        printf("mico_service_info_new failed\n");
        return -1;
    }

    // service_info_dependencies dp = {.name = "test1"};
    // mico_service_add_dependency(ms, &dp);

    ret = mico_service_sub(ms, "sendto_t2", ms, sendto_t2_callback);
    if (ret != 0) {
        printf("mico_service_sub failed\n");
        return -1;
    }

    mico_service_run_thread(ms);

    printf("Test2 ready. Waiting for messages...\n");
    while (1) {
        sleep(1);
    }
    return 0;
}