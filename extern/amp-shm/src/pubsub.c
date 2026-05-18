#include "pubsub.h"
#include "cJSON.h"
#include <stdio.h>
#include <string.h>
#include "shm_comm.h"
#include <sys/shm.h>
#include <stdlib.h>
#include <time.h>  // for nanosleep
#include "om.h"
#include "json_config_processor.h"
#include "shm_log.h"
#include "shm_errors.h"

// 内部类型定义
typedef vec_t(shm_endpoint_t*) vec_shm_endpoint_ptr_t;

// 函数声明（按功能分组）
int gen_topic_puber_channel_name(char* buffer, const char* topic, const char* puber);
int config_pub_sub(pubsub_config_new_t* pcfg);
int cmp_vec_topic(const void* a, const void* b);

/**
 * @brief 初始化发布订阅配置对象（新版本）
 * @param[in,out] pcfg 发布订阅配置对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 * @pre pcfg != NULL
 * @post 配置对象被初始化为空状态
 */
int pubsub_config_new_init(pubsub_config_new_t* pcfg)
{
    // 参数验证
    if (pcfg == NULL) {
        log_error("Invalid parameter: pcfg is NULL");
        return SHM_INVALID_PARAM;
    }
    
    // 初始化JSON配置对象
    json_config_init(&pcfg->pcfg_json_obj);
    
    // 初始化共享内存通信配置
    pcfg->shm_comm_config.channel_num = 0;
    pcfg->shm_comm_config.ep_table = NULL;
    
    // 初始化接收消息休眠延时为0（无休眠）
    pcfg->recv_sleep_ns = 0;
    
    log_info("Successfully initialized pubsub_config_new_t object");
    return SHM_SUCCESS;
}

/**
 * @brief 释放发布订阅配置对象（新版本）
 * @param[in,out] pcfg 发布订阅配置对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 * @pre pcfg != NULL
 * @post 配置对象被清理，所有资源被释放
 */
int pubsub_config_new_deinit(pubsub_config_new_t* pcfg)
{
    // 参数验证
    if (pcfg == NULL) {
        log_error("Invalid parameter: pcfg is NULL");
        return SHM_INVALID_PARAM;
    }
    
    // 释放JSON配置对象
    json_config_deinit(&pcfg->pcfg_json_obj);
    
    // 共享内存通信配置会在shm_comm_deinit中自动清理
    log_info("Successfully deinitialized pubsub_config_new_t object");
    return SHM_SUCCESS;
}

/**
 * @brief 生成主题-发布者通道名称
 * @param[out] buffer 输出缓冲区
 * @param[in] topic 主题名称
 * @param[in] puber 发布者名称
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 * @pre buffer != NULL, topic != NULL, puber != NULL
 * @post 缓冲区包含格式化的通道名称
 */
int gen_topic_puber_channel_name(char* buffer, const char* topic, const char* puber)
{
    if (buffer == NULL || topic == NULL || puber == NULL) {
        log_error("Invalid parameters: buffer=%p, topic=%p, puber=%p", buffer, topic, puber);
        return SHM_INVALID_PARAM;
    }
    
    int ret = snprintf(buffer, MAX_ENDPOINT_NAME, "[topic]%s:%s", topic, puber);
    if (ret < 0 || ret >= MAX_ENDPOINT_NAME) {
        log_error("Failed to generate channel name: ret=%d, topic=%s, puber=%s", ret, topic, puber);
        return SHM_COMM_BUFFER_TOO_SMALL;
    }
    
    return SHM_SUCCESS;
}

/**
 * @brief 接收消息（阻塞式）
 * @param[in] pcfg 发布订阅配置对象指针
 * @param[in] recver_name 接收者名称
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 * @note 此函数为阻塞调用，会持续接收消息直到进程退出
 */
int recv_msg(pubsub_config_new_t* pcfg, const char* recver_name)
{
    // 参数验证
    if (pcfg == NULL || recver_name == NULL) {
        log_error("Invalid parameters: pcfg=%p, recver_name=%p", pcfg, recver_name);
        return SHM_INVALID_PARAM;
    }

    // 获取端点表指针
    shm_endpoint_table_t* ep_table = shm_get_endpoint_table(&pcfg->shm_comm_config);
    if (ep_table == NULL) {
        log_error("Failed to get endpoint table");
        return SHM_INVALID_CONFIG;
    }

    int result = SHM_SUCCESS;
    char tmp[MAX_ENDPOINT_NAME] = {0};
    vec_shm_endpoint_ptr_t vsep;
    vec_init(&vsep);

    // 查找接收者对应的所有端点
    int json_config_center_i = 0;
    json_config_center_t* json_config_center_iter;
    vec_foreach_ptr(&pcfg->pcfg_json_obj.center, json_config_center_iter, json_config_center_i) {
        int json_config_center_subers_i = 0;
        json_config_suber_t *json_config_center_subers_iter;
        vec_foreach_ptr(&json_config_center_iter->subers, json_config_center_subers_iter, json_config_center_subers_i)
        {
            if (strcmp(json_config_center_subers_iter->name, recver_name) == 0)
            {
                int json_config_center_pubers_i = 0;
                json_config_puber_t *json_config_center_pubers_iter;
                vec_foreach_ptr(&json_config_center_iter->pubers, json_config_center_pubers_iter, json_config_center_pubers_i)
                {
                    // 生成通道名称并获取端点
                    int ret = gen_topic_puber_channel_name(tmp, json_config_center_iter->topic, 
                                                         json_config_center_pubers_iter->name);
                    if (ret != SHM_SUCCESS) {
                        log_error("Failed to generate channel name for topic=%s, puber=%s", 
                                 json_config_center_iter->topic, json_config_center_pubers_iter->name);
                        result = ret;
                        goto cleanup;
                    }

                    shm_endpoint_t *endpoint = shm_comm_get_channel(&pcfg->shm_comm_config, tmp, json_config_center_subers_iter->name);
                    if (endpoint != NULL) {
                        vec_push(&vsep, endpoint);
                        log_info("Added endpoint: %s->%s", tmp, json_config_center_subers_iter->name);
                    } else {
                        log_error("Endpoint is null! %s->%s", tmp, json_config_center_subers_iter->name);
                        result = SHM_ENDPOINT_NOT_FOUND;
                        goto cleanup;
                    }
                }
            }
        }
    }

    // 如果没有找到任何端点，直接返回错误
    if (vsep.length == 0) {
        log_error("No endpoints found for receiver: %s", recver_name);
        result = SHM_ENDPOINT_NOT_FOUND;
        goto cleanup;
    }

    // 主消息接收循环
    log_info("Starting message reception for receiver: %s", recver_name);
    while (true) {
        int shm_endpoint_ptr_i = 0;
        shm_endpoint_t* ep = NULL;
        vec_foreach(&vsep, ep, shm_endpoint_ptr_i) {
            // 接收零拷贝消息
            shm_base_msg_t *shmsg = shm_comm_recv_zc(ep_table, ep);
            if (shmsg != NULL) {
                PubSubHdr *psh = (PubSubHdr *)shmsg->payload;
                log_debug("Received message: len=%d, topic=%s, msg_size=%d", 
                         shmsg->size, psh->topic, psh->size);

                // 查找对应的OM主题
                om_topic_t *om_topic = om_find_topic(psh->topic, 0);
                if (om_topic == NULL) {
                    log_error("Topic not found: %s, discarding packet", psh->topic);
                    shm_comm_recv_zc_end(ep);
                    continue;
                }

                // 发布消息到OM系统
                int om_result = om_publish(om_topic, psh->payload, psh->size, true, false);
                if (om_result != 0) {
                    log_warn("Failed to publish message to OM topic %s, result=%d", psh->topic, om_result);
                }
                shm_comm_recv_zc_end(ep);
            } else {
                // 没有消息时根据配置进行休眠，避免CPU占用过高
                if (pcfg->recv_sleep_ns > 0) {
                    struct timespec ts = { .tv_sec = 0, .tv_nsec = pcfg->recv_sleep_ns };
                    nanosleep(&ts, NULL);
                }
            }
        }
    }

cleanup:
    // 清理资源
    vec_deinit(&vsep);
    return result;
}

/**
 * @brief 发布消息到指定主题
 * @param[in] pcfg 发布订阅配置对象指针
 * @param[in] puber_name 发布者名称
 * @param[in] topic 主题名称
 * @param[in] msg 消息数据指针
 * @param[in] len 消息数据长度
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int pub_msg(pubsub_config_new_t* pcfg, const char* puber_name, const char* topic, const void* msg, uint32_t len)
{
    // 参数验证
    if (pcfg == NULL || puber_name == NULL || topic == NULL || msg == NULL || len == 0) {
        log_error("Invalid parameters: pcfg=%p, puber_name=%p, topic=%p, msg=%p, len=%u", 
                 pcfg, puber_name, topic, msg, len);
        return SHM_INVALID_PARAM;
    }

    // 获取端点表指针
    shm_endpoint_table_t* ep_table = shm_get_endpoint_table(&pcfg->shm_comm_config);
    if (ep_table == NULL) {
        log_error("Failed to get endpoint table");
        return SHM_INVALID_CONFIG;
    }

    int result = SHM_SUCCESS;
    int error_count = 0, send_count = PUB_HAVE_NO_TOPIC;
    char tmp[MAX_ENDPOINT_NAME] = {0};

    // 遍历所有配置中心，查找匹配的主题
    int json_config_center_i = 0;
    json_config_center_t* json_config_center_iter;
    vec_foreach_ptr(&pcfg->pcfg_json_obj.center, json_config_center_iter, json_config_center_i) {
        if (strcmp(json_config_center_iter->topic, topic) == 0) {
            int json_config_center_subers_i = 0;
            json_config_suber_t *json_config_center_subers_iter;
            
            // 遍历所有订阅者
            vec_foreach_ptr(&json_config_center_iter->subers, json_config_center_subers_iter, json_config_center_subers_i) {
                // 生成通道名称
                int ret = gen_topic_puber_channel_name(tmp, topic, puber_name);
                if (ret != SHM_SUCCESS) {
                    log_error("Failed to generate channel name for topic=%s, puber=%s", topic, puber_name);
                    error_count++;
                    continue;
                }

                // 获取通信端点
                shm_endpoint_t* endpoint = shm_comm_get_channel(&pcfg->shm_comm_config, tmp, json_config_center_subers_iter->name);
                if (endpoint == NULL) {
                    log_error("Endpoint not found: topic=%s, puber=%s, suber=%s", 
                             topic, puber_name, json_config_center_subers_iter->name);
                    error_count++;
                    continue;
                }
                
                // 分配发送缓冲区
                shm_base_msg_t* base_msg = shm_comm_send_alloc(ep_table, endpoint, len + sizeof(PubSubHdr));
                if (base_msg == NULL) {
                    log_error("Send buffer full: topic=%s, puber=%s, suber=%s", 
                             topic, puber_name, json_config_center_subers_iter->name);
                    error_count++;
                    continue;
                }

                // 填充消息内容
                base_msg->type = MSG_TYPE_PUB_SUB;
                PubSubHdr *psh = (PubSubHdr *)base_msg->payload;
                
                // 安全拷贝主题名称
                strncpy(psh->topic, topic, MAX_TOPIC_LEN - 1);
                psh->topic[MAX_TOPIC_LEN - 1] = '\0'; // 确保字符串终止
                
                // 拷贝消息负载
                memcpy(psh->payload, msg, len);
                psh->size = len;
                
                // 完成发送
                shm_comm_send_alloc_end(ep_table, endpoint);
                // 确保
                (send_count == PUB_HAVE_NO_TOPIC) ? (send_count = 1) : (send_count++);
                log_debug("Message published: topic=%s, puber=%s, suber=%s, size=%u", 
                         topic, puber_name, json_config_center_subers_iter->name, len);
            }
        }
    }

    // 发布时出现错误，若error_count为0，则是PUB_HAVE_NO_TOPIC错误
    if (error_count > 0 || send_count == PUB_HAVE_NO_TOPIC) {
        log_warn("Message publication completed with %d errors", error_count);
        result = PUB_MSG_HAVE_FAILED;
    }

    return result;
}


int cmp_vec_topic(const void* a, const void* b)
{
    const json_config_center_t* pa = a;
    const json_config_center_t* pb = b;
    return pb->topic_priority - pa->topic_priority;
}

/**
 * @brief 配置发布订阅通道
 * @param[in] pcfg 发布订阅配置对象指针
 * @return 成功返回SHM_SUCCESS，失败返回错误码
 */
int config_pub_sub(pubsub_config_new_t* pcfg)
{
    // 参数验证
    if (pcfg == NULL) {
        log_error("Invalid parameter: pcfg is NULL");
        return SHM_INVALID_PARAM;
    }

    char tmp[MAX_ENDPOINT_NAME] = {0};
    
    // 按主题优先级排序配置
    vec_sort(&pcfg->pcfg_json_obj.center, cmp_vec_topic);

    int json_config_center_i = 0;
    json_config_center_t* json_config_center_iter;
    vec_foreach_ptr(&pcfg->pcfg_json_obj.center, json_config_center_iter, json_config_center_i) {
        // 查找或创建OM主题
        // 因为存在一个包头用来记录主题，所以这里的队列大小创建时要增加
        json_config_center_iter->msg_size += sizeof(PubSubHdr);
        om_topic_t *topic = om_core_find_topic(json_config_center_iter->topic, 0);
        if (topic == NULL) {
            topic = om_config_topic(NULL, "a", json_config_center_iter->topic, json_config_center_iter->msg_size);
            if (topic == NULL) {
                log_error("Failed to create OM topic: %s", json_config_center_iter->topic);
                return SHM_INTERNAL_ERROR;
            }
        } else {
            log_warn("Topic [%s] already configured, skipping", json_config_center_iter->topic);
        }
        
        // 配置所有订阅者
        int json_config_center_subers_i = 0;
        json_config_suber_t* json_config_center_subers_iter;
        vec_foreach_ptr(&json_config_center_iter->subers, json_config_center_subers_iter, json_config_center_subers_i) {
            int json_config_center_pubers_i = 0;
            json_config_puber_t *json_config_center_pubers_iter;
            
            // 配置所有发布者
            vec_foreach_ptr(&json_config_center_iter->pubers, json_config_center_pubers_iter, json_config_center_pubers_i) {
                // 生成通道名称
                int ret = gen_topic_puber_channel_name(tmp, json_config_center_iter->topic,
                                                     json_config_center_pubers_iter->name);
                if (ret != SHM_SUCCESS) {
                    log_error("Failed to generate channel name for topic=%s, puber=%s",
                             json_config_center_iter->topic, json_config_center_pubers_iter->name);
                    return ret;
                }

                // 添加共享内存通道配置
                ret = shm_comm_config_add_channel(&pcfg->shm_comm_config, tmp, 
                        json_config_center_subers_iter->name, 
                        json_config_center_iter->msg_size,
                        json_config_center_iter->len);
                
                if (ret != SHM_SUCCESS) {
                    log_error("Configuration conflict for channel %s->%s, ret=%d",
                             tmp, json_config_center_subers_iter->name, ret);
                    return ret;
                }
                
                log_info("Configured channel: %s -> %s (msg_size=%d, len=%d)",
                        tmp, json_config_center_subers_iter->name,
                        json_config_center_iter->msg_size, json_config_center_iter->len);
            }
        }
    }
    
    return SHM_SUCCESS;
}

int pub_init_config_files(pubsub_config_new_t* pcfg, const char** filenames, int count)
{
    // 参数验证
    if (pcfg == NULL || filenames == NULL || count <= 0) {
        log_error("Invalid parameters: pcfg=%p, filenames=%p, count=%d", pcfg, filenames, count);
        return SHM_INVALID_PARAM;
    }

    // 初始化shm_comm_config
    pcfg->shm_comm_config.channel_num = 0;
    pcfg->shm_comm_config.ep_table = NULL;

    int result = SHM_SUCCESS;
    
    // 使用新的JSON配置处理模块解析和合并配置文件
    // json_config_t json_config;
    result = json_config_merge_files(&pcfg->pcfg_json_obj, filenames, count, MERGE_STRATEGY_MERGE);
    if (result != SHM_SUCCESS) {
        log_error("Failed to merge config files, result=%d", result);
        return result;
    }

    log_info("Successfully parsed and merged %d config files", count);

    // 验证配置的有效性
    result = json_config_validate(&pcfg->pcfg_json_obj);
    if (result != SHM_SUCCESS) {
        log_error("Config validation failed, result=%d", result);
        return result;
    }

    // 调试输出配置信息
    char* config_str = json_config_to_str(&pcfg->pcfg_json_obj);
    if (config_str != NULL) {
        log_debug("Final configuration: %s", config_str);
        free(config_str);
    }

    // 配置发布订阅通道
    result = config_pub_sub(pcfg);
    if (result != SHM_SUCCESS) {
        log_error("Failed to configure pub/sub channels, ret=%d", result);
    }

    return result;
}

int pub_init_config_strs(pubsub_config_new_t* pcfg, const char** json_strs, int count)
{
    // 参数验证
    if (pcfg == NULL || json_strs == NULL || count <= 0) {
        log_error("Invalid parameters: pcfg=%p, json_strs=%p, count=%d", pcfg, json_strs, count);
        return SHM_INVALID_PARAM;
    }

    // 初始化shm_comm_config
    pcfg->shm_comm_config.channel_num = 0;
    pcfg->shm_comm_config.ep_table = NULL;

    int result = SHM_SUCCESS;
    
    result = json_config_merge_strs(&pcfg->pcfg_json_obj, json_strs, count, MERGE_STRATEGY_MERGE);
    if (result != SHM_SUCCESS) {
        log_error("Failed to merge JSON strings, result=%d", result);
        return result;
    }

    log_info("Successfully parsed and merged %d JSON config strings", count);

    // 验证配置的有效性
    result = json_config_validate(&pcfg->pcfg_json_obj);
    if (result != SHM_SUCCESS) {
        log_error("Config validation failed, result=%d", result);
        return result;
    }

    // 调试输出配置信息
    char* config_str = json_config_to_str(&pcfg->pcfg_json_obj);
    if (config_str != NULL) {
        log_debug("Final configuration: %s", config_str);
        free(config_str);
    }

    // 配置发布订阅通道
    result = config_pub_sub(pcfg);
    if (result != SHM_SUCCESS) {
        log_error("Failed to configure pub/sub channels, ret=%d", result);
    }

    return result;
}

int pubsub_cal_mem(pubsub_config_new_t* pcfg)
{
    if (pcfg == NULL) {
        log_error("Invalid parameter: pcfg is NULL");
        return SHM_INVALID_PARAM;
    }
    
    int mem_size = shm_comm_cal_mem(&pcfg->shm_comm_config);
    log_info("Calculated required shared memory size: %d bytes", mem_size);
    return mem_size;
}

int pubsub_init(pubsub_config_new_t* pcfg, int is_main, void* shm_addr, int mem_len)
{
    // 参数验证
    if (pcfg == NULL || shm_addr == NULL) {
        log_error("Invalid parameters: pcfg=%p, shm_addr=%p", pcfg, shm_addr);
        return SHM_INVALID_PARAM;
    }

    if (mem_len <= 0) {
        log_error("Invalid parameter: mem_len=%d", mem_len);
        return SHM_INVALID_PARAM;
    }

    log_info("Initializing pub/sub system with memory size: %d bytes", mem_len);
    
    int result = SHM_SUCCESS;
    
    if (is_main) {
        // 主进程初始化
        result = shm_comm_init_main(&pcfg->shm_comm_config, shm_addr, mem_len);
        if (result != SHM_SUCCESS) {
            log_error("Failed to initialize main process, ret=%d", result);
            return result;
        }
        
        result = shm_comm_load_config(&pcfg->shm_comm_config);
        if (result != SHM_SUCCESS) {
            log_error("Failed to load shared memory configuration, ret=%d", result);
        } else {
            log_info("Main process initialization completed successfully");
        }
    } else {
        // 从进程初始化
        result = shm_comm_init_minor(&pcfg->shm_comm_config, shm_addr);
        if (result != SHM_SUCCESS) {
            log_error("Failed to initialize minor process, ret=%d", result);
        } else {
            log_info("Minor process initialization completed successfully");
        }
    }
    
    return result;
}
