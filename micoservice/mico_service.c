#include "mico_service.h"
#include "snow_flake.h"
#include "base_types.h"
#include "cf_log.h"
#include "base_msg_json_tools.h"
#include "unistd.h"

/**
 * @brief 
 * 
 * @param r 
 * @param si 
 * @return int 
 *          1 : 没有把si存到map，需要释放
 *          0 : 把si存到map，不用释放
 */
int add_dependency_to_local(mico_service* ms, service_info si)
{
    int ret = -1;
    check_timeout(&si.timestamp, 0);
    service_local_cache* sc = map_get(&ms->dependency_cache, si.name);
    if (NULL == sc)
    {
        log_debug("name:%s, id:%s get", si.name, si.id);
        service_local_cache new_sc;
        map_init(&new_sc.msc);  // 初始化服务实例map
        new_sc.total = 0;       // 总实例数
        new_sc.count = 0;       // 轮询计数

        service_local_info sli;
        sli.si = si;
        new_sc.total += 1;      // 新增实例
        map_set(&new_sc.msc, si.id, sli);  // 添加实例到map
        map_set(&ms->dependency_cache, si.name, new_sc);  // 添加服务到依赖缓存
        ret = 0;
    }
    else
    {
        service_local_info* tmp_si = map_get(&sc->msc, si.id);
        if (NULL == tmp_si)
        {
            log_debug("name:%s, id:%s new add", si.name, si.id);
            service_local_info sli;
            sli.si = si;
            sc->total += 1;
            map_set(&sc->msc, si.id, sli);
            ret = 0;
        }
        else
        {
            log_debug("name:%s, id:%s check_timeout", si.name, si.id);
            check_timeout(&tmp_si->si.timestamp, 0);
            ret = 1;
        }
        
    }

    return ret;
}

/**
 * @brief 处理来自注册中心的消息
 * 
 * @param arg mico_service指针
 * @param msg 消息内容
 * @param len 消息长度
 * @return int 处理结果
 */
int ms_deal_registry_msg(void* arg, void* msg, uint32_t len)
{
    mico_service* ms = arg;
    ((char*)msg)[len] = '\0';
    log_trace("len:%d, %s", len, (char*)msg);
    base_msg bs_msg;
    base_msg_init(&bs_msg);
    if (BASE_SUCCESS == base_msg_deserialize_str(&bs_msg, msg)){
        switch (bs_msg.msg)
        {
        case MSG_TYPE_REGISTRY_TO_SERVICE:
            ;
            service_info si;
            service_info_init(&si);
            if (BASE_SUCCESS == service_info_deserialize_str(&si, msg))
            {
                int need_free_si = add_dependency_to_local(ms, si);
                if (need_free_si)
                {
                    service_info_deinit(&si);
                }
            }

            break;
        
        default:
            log_error("service: %s, unknow msg type:%d", ms->info->name, bs_msg.msg);
            break;
        }
    }
    base_msg_deinit(&bs_msg);
}

/**
 * @brief 添加服务依赖
 * 
 * @param ms mico_service指针
 * @param dependency 依赖信息
 * @return int 添加结果
 *          BASE_SUCCESS: 成功
 *          BASE_FAILED: 失败
 */
int mico_service_add_dependency(mico_service * ms, service_info_dependencies* dependency)
{
    if (NULL == ms || NULL == dependency || NULL == ms->info)
    {
        return BASE_FAILED;
    }
    
    service_info_dependencies dp;
    dp.name = strdup(dependency->name);
    vec_push(&ms->info->dependencies, dp);
    
    return BASE_SUCCESS;
}

/**
 * @brief 设置服务状态
 * 
 * @param ms mico_service指针
 * @param status 状态字符串
 */
void mico_service_set_status(mico_service * ms, char* status)
{
    if (NULL != ms->info->status)
    {
        free(ms->info->status);
    }
    
    ms->info->status = strdup(status);
}

/**
 * @brief 设置服务版本
 * 
 * @param ms mico_service指针
 * @param version 版本字符串
 */
void mico_service_set_version(mico_service * ms, char* version)
{
    if (NULL != ms->info->version)
    {
        free(ms->info->version);
    }
    ms->info->version = strdup(version);
}

/**
 * @brief 设置服务端点
 * 
 * @param ms mico_service指针
 * @param endpoint 端点字符串
 */
void mico_service_set_endpoint(mico_service * ms, char* endpoint)
{
    if (NULL != ms->info->endpoint)
    {
        free(ms->info->endpoint);
    }
    ms->info->endpoint = strdup(endpoint);
}

int mico_service_node(mico_service * ms, char* node_json_file)
{
    int ret = 0;
    if ((ret = node_deserialize_file(&ms->nd, node_json_file)) != BASE_SUCCESS) {
        log_warn("Failed to load node config from: %s, ret: %d", node_json_file, ret);
    }
    if (NULL != ms->nd.id)
    {
        if (NULL != ms->info->node_id)
        {
            free(ms->info->node_id);
        }
        
        ms->info->node_id = strdup(ms->nd.id);
        return BASE_SUCCESS;
    }
    
    return ret;
}

void ms_distributed_log_callback(log_Event *ev)
{
    mico_service * ms = ev->udata;
    char log_msg[4096];
    char buf[64];
    buf[strftime(buf, sizeof(buf), "%H:%M:%S", ev->time)] = '\0';
    int len = snprintf(
        log_msg, sizeof(log_msg), "[%s] %s %-5s %s:%d: ", ms->info->name, 
        buf, level_strings[ev->level], ev->file, ev->line);
    if (len < sizeof(log_msg)) {
        vsnprintf(log_msg + len, sizeof(log_msg) - len, ev->fmt, ev->ap);
    }
    // 确保字符串以换行符结尾
    int total_len = strlen(log_msg);
    if (total_len > 0 && log_msg[total_len - 1] != '\n' && total_len < sizeof(log_msg) - 1) {
        log_msg[total_len] = '\n';
        log_msg[total_len + 1] = '\0';
        total_len++;
    }
    ms->rc->pub_msg(ms->rc, REGISTRY_ID, log_msg, total_len);
}

/**
 * @brief 创建新的mico_service实例
 * 
 * @param service_name 服务名称
 * @param rct 注册中心连接信息
 * @return mico_service* 新创建的mico_service实例
 */
mico_service *mico_service_new(char* service_name, abs_comm_connect* rct)
{
    mico_service * ms = malloc(sizeof(mico_service));

    char* rand_id = id_generate_string();

    ms->rc = abs_comm_new(rand_id, rct);

    abs_comm_sub(ms->rc, rand_id, ms, ms_deal_registry_msg);

    if (NULL == ms->rc)
    {
        free(ms);
        free(rand_id);
        return NULL;
    }
    
    ms->info = malloc(sizeof(service_info));
    service_info_init(ms->info);
    ms->info->name = strdup(service_name);
    ms->info->id = strdup(rand_id);
    ms->info->status = strdup("UP");
    ms->info->version = strdup("1.0.0");
    ms->info->endpoint = strdup("test1");
    ms->info->node_id = strdup("none");
    ms->info->msg = MSG_TYPE_SERVICE_INFO;
    ms->thread_run_flag = 0;

    map_init(&ms->dependency_cache);
    node_init(&ms->nd);

    free(rand_id);
    return ms;
}

mico_service *mico_service_info_str_new(char* service_info_str, abs_comm_connect* rct)
{
    mico_service * ms = malloc(sizeof(mico_service));
    ms->info = malloc(sizeof(service_info));
    service_info_init(ms->info);
    if (BASE_SUCCESS == service_info_deserialize_str(ms->info, service_info_str))
    {
        ms->rc = abs_comm_new(ms->info->id, rct);
        
        if (NULL == ms->rc)
        {
            service_info_deinit(ms->info);
            free(ms);
            return NULL;
        }

        int ret = abs_comm_sub(ms->rc, ms->info->id, ms, ms_deal_registry_msg);

        if (BASE_SUCCESS != ret)
        {
            service_info_deinit(ms->info);
            free(ms);
            return NULL;
        }
        ms->info->msg = MSG_TYPE_SERVICE_INFO;
        ms->thread_run_flag = 0;

        map_init(&ms->dependency_cache);
        node_init(&ms->nd);

        log_add_callback(ms_distributed_log_callback, ms, LOG_TRACE);

        return ms;
    }else
    {
        log_error("service_info_str deserialize failed");
        return NULL;
    }
}

mico_service *mico_service_info_new(char* service_info_file, abs_comm_connect* rct)
{
    mico_service * ms = malloc(sizeof(mico_service));
    ms->info = malloc(sizeof(service_info));
    service_info_init(ms->info);
    if (BASE_SUCCESS == service_info_deserialize_file(ms->info, service_info_file))
    {
        ms->rc = abs_comm_new(ms->info->id, rct);

        if (NULL == ms->rc)
        {
            service_info_deinit(ms->info);
            free(ms);
            return NULL;
        }

        int ret = abs_comm_sub(ms->rc, ms->info->id, ms, ms_deal_registry_msg);

        if (BASE_SUCCESS != ret)
        {
            service_info_deinit(ms->info);
            free(ms);
            return NULL;
        }
        ms->info->msg = MSG_TYPE_SERVICE_INFO;
        ms->thread_run_flag = 0;

        map_init(&ms->dependency_cache);
        node_init(&ms->nd);

        pthread_mutex_init(&ms->mutex_log, NULL);
        log_set_lock(log_mutex_lock, &ms->mutex_log);

        log_add_callback(ms_distributed_log_callback, ms, LOG_TRACE);

        return ms;
    }else
    {
        log_error("service_info_str deserialize failed");
        return NULL;
    }
}

/**
 * @brief 运行服务主循环
 * 
 * @param arg mico_service指针
 */
void mico_service_run(void *arg)
{
    mico_service *ms = arg;
    ms->thread_run_flag = THREAD_RUNING;
    while (THREAD_RUNING == ms->thread_run_flag)
    {
        char* tmp = service_info_to_str(ms->info);
        ms->rc->pub_msg(ms->rc, REGISTRY_ID, tmp, strlen(tmp));
        free(tmp);
        sleep(1);
    }
    
}

/**
 * @brief 获取服务实例
 * 
 * @param ms mico_service指针
 * @param service_name 服务名称
 * @param method 获取方法
 *          METHOD_RANDOM: 随机获取
 *          METHOD_ONE: 顺序获取
 * @return service_info* 服务实例指针
 */
service_info* mico_service_get_instance(mico_service *ms, char* service_name, int method)
{
    service_local_cache *sc = map_get(&ms->dependency_cache, service_name);
    if (sc && sc->total > 0)
    {
        // 先检查所有实例是否超时
        const char *key;
        map_iter_t si_iter = map_iter(&sc->msc);
        while ((key = map_next(&sc->msc, &si_iter))) {
            service_local_info *sli = map_get(&sc->msc, key);
            if (sli && check_timeout(&sli->si.timestamp, 5)) {  // 检查5秒超时
                log_debug("service %s instance %s timeout, removing", service_name, sli->si.id);
                service_info_deinit(&sli->si);  // 释放超时实例
                map_remove(&sc->msc, key);      // 从map中移除
                sc->total--;                    // 更新实例总数
            }
        }

        if (sc->total == 0) {
            return NULL;  // 没有可用实例
        }

        switch (method)
        {
        case METHOD_RANDOM: {
            // 随机获取一个实例
            int index = rand() % sc->total;  // 生成随机索引
            si_iter = map_iter(&sc->msc);
            for (int i = 0; (key = map_next(&sc->msc, &si_iter)); i++) {
                if (i == index) {
                    service_local_info *sli = map_get(&sc->msc, key);
                    return &sli->si;  // 返回随机选择的实例
                }
            }
            break;
        }
        case METHOD_ONE: {
            // 按顺序获取实例
            si_iter = map_iter(&sc->msc);
            for (int i = 0; (key = map_next(&sc->msc, &si_iter)); i++) {
                if (i == sc->count) {
                    service_local_info *sli = map_get(&sc->msc, key);
                    sc->count = (sc->count + 1) % sc->total; // 轮询计数更新
                    return &sli->si;  // 返回当前实例
                }
            }
            break;
        }
        default:
            log_error("Unknown method: %d", method);
            break;
        }
    }
    return NULL;
}

void mico_service_run_thread(mico_service *ms)
{
    pthread_create(&ms->pid, NULL, (void *)mico_service_run, (void*)ms);
}

void mico_service_stop(mico_service *ms)
{
    ms->thread_run_flag = THREAD_STOPING;
    pthread_join(ms->pid, NULL);
}

/**
 * @brief 检查并清理超时实例
 * 
 * @param ms mico_service指针
 */
void mico_service_check_timeout(mico_service *ms)
{
    const char *key;
    map_iter_t cluster_iter = map_iter(&ms->dependency_cache);  // 遍历所有服务
    while ((key = map_next(&ms->dependency_cache, &cluster_iter)))
    {
        service_local_cache *sc = map_get(&ms->dependency_cache, key);
        if (sc)
        {
            map_iter_t si_iter = map_iter(&sc->msc);  // 遍历服务实例
            while ((key = map_next(&sc->msc, &si_iter)))
            {
                service_local_info* sli = map_get(&sc->msc, key);
                if (sli && check_timeout(&sli->si.timestamp, 5))  // 检查5秒超时
                {
                    log_debug("service %s instance %s timeout, removing", sli->si.name, sli->si.id);
                    service_info_deinit(&sli->si);  // 释放超时实例
                    map_remove(&sc->msc, key);      // 从map中移除
                    sc->total--;                    // 更新实例总数
                }
            }
        }
    }
}

/**
 * @brief 删除mico_service实例
 * 
 * @param ms mico_service指针
 */
void mico_service_delete(mico_service *ms)
{
    abs_comm_delete(ms->rc);
    service_info_deinit(ms->info);

    const char *key;
    map_iter_t cluster_iter = map_iter(&ms->dependency_cache);
    while ((key = map_next(&ms->dependency_cache, &cluster_iter)))
    {
        service_local_cache *sc = map_get(&ms->dependency_cache, key);
        if (sc)
        {
            map_iter_t si_iter = map_iter(&sc->msc);
            while ((key = map_next(&sc->msc, &si_iter)))
            {
                service_local_info* sli = map_get(&sc->msc, key);
                if (sli)
                {
                    log_debug("service cache free, name:%s, id:%s", sli->si.name, sli->si.id);
                    service_info_deinit(&sli->si);
                }
            }
            map_deinit(&sc->msc);
        }
    }
    map_deinit(&ms->dependency_cache);
    node_deinit(&ms->nd);

    free(ms->info);
    free(ms);
}
