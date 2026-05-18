#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <pthread.h>
#include "mico_service.h"
#include <unistd.h>

// 高精度时间获取函数 (微秒)
static uint64_t get_timestamp_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

// 延时测试全局变量
volatile int g_received = 0;
volatile uint64_t g_end_time = 0;

// 接收回调函数
int recvfrom_t2_callback(void* arg, void* msg, uint32_t len) {
    g_end_time = get_timestamp_us();
    g_received = 1;
    return 0;
}

// 延时测试函数
void test_latency(mico_service* ms, int msg_size, int test_count) {
    char* send_buf = malloc(msg_size);
    if (!send_buf) {
        perror("malloc failed");
        return;
    }
    memset(send_buf, 'A', msg_size); // 'A' 表示延时测试
    
    uint64_t total_time = 0;
    int success_count = 0;
    uint64_t max_delay = 0, min_delay = UINT64_MAX;

    printf("\n=== Starting Latency Test ===\n");
    printf("Message size: %d bytes\n", msg_size);
    printf("Test count: %d\n", test_count);
    
    for (int i = 0; i < test_count; i++) {
        g_received = 0;
        uint64_t start_time = get_timestamp_us();
        
        int ret = mico_service_pub(ms, "sendto_t2", send_buf, msg_size);
        if (ret != 0) {
            printf("Send failed: %d\n", ret);
            continue;
        }
        
        // 等待回复 (带超时机制)
        int wait_count = 0;
        while (!g_received && wait_count < 1000000) {
            usleep(1);
            wait_count++;
        }
        
        if (!g_received) {
            printf("Timeout waiting for reply\n");
            continue;
        }
        
        uint64_t rtt = g_end_time - start_time; // 往返时间
        uint64_t one_way = rtt / 2;            // 单程延时
        
        total_time += one_way;
        if (one_way > max_delay) max_delay = one_way;
        if (one_way < min_delay) min_delay = one_way;
        success_count++;
    }

    if (success_count > 0) {
        double avg_latency = (double)total_time / success_count;
        printf("\nLatency Test Results:\n");
        printf("Success rate: %d/%d (%.2f%%)\n", 
              success_count, test_count, 
              100.0 * success_count / test_count);
        printf("Average latency: %.2f us\n", avg_latency);
        printf("Max latency: %lu us\n", max_delay);
        printf("Min latency: %lu us\n", min_delay);
    } else {
        printf("No successful latency measurements\n");
    }
    
    free(send_buf);
}

// 吞吐量测试函数
void test_throughput(mico_service* ms, int msg_size, int duration_sec) {
    char* send_buf = malloc(msg_size);
    if (!send_buf) {
        perror("malloc failed");
        return;
    }
    memset(send_buf, 'B', msg_size); // 'B' 表示吞吐量测试
    
    uint64_t start_time = get_timestamp_us();
    uint64_t end_time = start_time + duration_sec * 1000000ULL;
    uint64_t total_bytes = 0;
    int msg_count = 0, failed_count = 0;

    printf("\n=== Starting Throughput Test ===\n");
    printf("Message size: %d bytes\n", msg_size);
    printf("Duration: %d seconds\n", duration_sec);
    
    while (get_timestamp_us() < end_time) {
        int ret = mico_service_pub(ms, "sendto_t2", send_buf, msg_size);
        if (ret != 0) {
            failed_count++;
            continue;
        }
        total_bytes += msg_size;
        msg_count++;
    }
    
    double elapsed_sec = (get_timestamp_us() - start_time) / 1000000.0;
    double throughput_mbps = (total_bytes * 8) / (elapsed_sec * 1000000);
    double msg_per_sec = msg_count / elapsed_sec;

    printf("\nThroughput Test Results:\n");
    printf("Total messages: %d\n", msg_count);
    printf("Failed messages: %d\n", failed_count);
    printf("Total bytes: %lu (%.2f MB)\n", 
          total_bytes, (double)total_bytes / (1024 * 1024));
    printf("Throughput: %.2f Mbps\n", throughput_mbps);
    printf("Message rate: %.2f msg/s\n", msg_per_sec);
    
    free(send_buf);
}

int main(int argc, char const *argv[]) {
    // if (argc < 2) {
    //     printf("Usage: %s [latency|throughput] [size] [count|duration]\n", argv[0]);
    //     printf("Example:\n");
    //     printf("  %s latency 64 1000    # 64-byte messages, 1000 tests\n");
    //     printf("  %s throughput 1024 5  # 1KB messages, 5-second test\n");
    //     return -1;
    // }
    
    int ret = 0;
    abs_comm_connect rct = {.ip = "127.0.0.1", .port = 19999};
    mico_service* ms = mico_service_info_new("test1.json", &rct);
    if (ms == NULL) {
        printf("mico_service_info_new failed\n");
        return -1;
    }

    // service_info_dependencies dp = {.name = "test2"};
    // mico_service_add_dependency(ms, &dp);

    ret = mico_service_sub(ms, "recvfrom_t2", NULL, recvfrom_t2_callback);
    if (ret != 0) {
        printf("mico_service_sub failed\n");
        return -1;
    }

    mico_service_run_thread(ms);
    sleep(1); // 等待连接建立

    // char* test_type = argv[1];
    // int size = atoi(argv[2]);
    // int value = atoi(argv[3]);

    // if (strcmp(test_type, "latency") == 0) {
    //     test_latency(ms, size, value);
    // } else if (strcmp(test_type, "throughput") == 0) {
    //     test_throughput(ms, size, value);
    // } else {
    //     printf("Unknown test type: %s\n", test_type);
    // }

    test_latency(ms, 64, 1000);
    sleep(1);
    test_latency(ms, 1024, 1000);
    sleep(1);
    test_latency(ms, 4096, 1000);
    sleep(1);
    test_throughput(ms, 1024, 5);
    sleep(1);
    test_throughput(ms, 4096, 5);

    while (1) {
        sleep(1);
    }
    return 0;
}