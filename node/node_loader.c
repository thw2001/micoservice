#include "node_loader.h"
#include "cf_log.h"
#include "node_json_tools.h"
#include "base_msg_json_tools.h"
#include "deploy_json_tools.h"
#include "filetrans_json_tools.h"
#include <stdlib.h>
#include <unistd.h>
#include "rpc.h"
#include <fcntl.h>
#include "uintptr2string.h"

//node_loader需要在核态？？

/**
 * @brief 分割字符串为适合 execv() 使用的 char ** 数组
 *        这个分隔会把数组的最后一个置为NULL
 * 
 * @param str 
 * @return char** 需要调用函数free_argv去free
 */
char **split_string_to_argv(const char *str) {
    // 动态分配空间存储分割后的字符串数组
    size_t num_tokens = 0;
    char *token;
    const char *delimiters = " \t\n";  // 分隔符

    // 计算字符串中有多少个分隔符分隔的项
    token = strtok((char *)str, delimiters);
    while (token != NULL) {
        num_tokens++;
        token = strtok(NULL, delimiters);
    }

    // 再次初始化 strtok()，这次是为了实际分割
    token = strtok((char *)str, delimiters);

    // 分配内存给字符串数组
    char **argv = malloc((num_tokens + 1) * sizeof(char *));
    // if (argv == NULL) {
    //     fprintf(stderr, "Memory allocation failed for argv array.\n");
    //     exit(EXIT_FAILURE);
    // }

    // 分配内存给每个字符串并填充 argv 数组
    for (size_t i = 0; i < num_tokens; i++) {
        argv[i] = strdup(token);
        // if (argv[i] == NULL) {
        //     fprintf(stderr, "Memory allocation failed for token %zu.\n", i);
        //     exit(EXIT_FAILURE);
        // }
        token = strtok(NULL, delimiters);
    }

    // 设置最后一个元素为 NULL
    argv[num_tokens] = NULL;

    return argv;
}

/**
 * @brief 释放由 split_string_to_argv() 分配的 argv 数组
 * 
 * @param argv 需要释放的字符串数组
 */
void free_argv(char **argv) {
    if (!argv) return;
    
    // 释放每个字符串
    for (int i = 0; argv[i] != NULL; i++) {
        free(argv[i]);
    }
    
    // 释放数组本身
    free(argv);
}

int node_loader_exec(const char *__path, char *const *__argv)
{
#if defined(__LINUX__)
    unsigned long ret = fork();
	if(ret == 0)
	{
		int res = execv(__path, __argv);
		return res;
	}
    return ret;
#elif defined(__REWORKS_RTP__)
    int res = execv(__path, __argv);
    return res;
#endif
}
//命令形式调用和处理，暂不用
int node_loader_deal_center_msg(void* arg, void* msg, uint32_t len)
{
    node_loader* loader = (node_loader*) arg;
    log_trace("len:%d, %s", len, (char*)msg);
    base_msg bs_msg;
    base_msg_init(&bs_msg);
    if (BASE_SUCCESS == base_msg_deserialize_str(&bs_msg, msg)){
        switch (bs_msg.msg)
        {
        
        default:
            log_error("unknow msg type:%d",bs_msg.msg);
            break;
        }
    }
    base_msg_deinit(&bs_msg);
    return BASE_SUCCESS;
}


void nl_distributed_log_callback(log_Event *ev)
{
    node_loader * loader = ev->udata;
    char log_msg[4096];
    char buf[64];
    buf[strftime(buf, sizeof(buf), "%H:%M:%S", ev->time)] = '\0';
    int len = snprintf(
        log_msg, sizeof(log_msg), "[%s] %s %-5s %s:%d: ", loader->n.name, 
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
    loader->rc->pub_msg(loader->rc, NODE_CENTER, log_msg, total_len);
}

//部署命令的rpc回调
static cJSON* deploy_cmd_rpc_handler(rpc_json_context* ctx, cJSON* params, cJSON* id) {
    // params为deploy_config对象
    deploy_config dc;
    deploy_config_init(&dc);
    int ret = deploy_config_deserialize(params, &dc);
    cJSON* result = cJSON_CreateObject();
    if (ret == 0 && dc.command) {
        char** argv = split_string_to_argv(dc.command);
        int exec_ret = node_loader_exec(argv[0], argv);
        log_debug("[rpc] exec: %s, ret: %d", dc.command, exec_ret);
        cJSON_AddNumberToObject(result, "exit_code", exec_ret);
        free_argv(argv);
    } else {
        ctx->error_code = JRPC_INVALID_PARAMS;
        ctx->error_message = "Invalid deploy_config params";
        cJSON_AddStringToObject(result, "error", "Invalid deploy_config params");
    }
    deploy_config_deinit(&dc);
    return result;
}

//文件传输命令的rpc回调
static cJSON* filetrans_cmd_rpc_handler(rpc_json_context* ctx,cJSON* params, cJSON* id){
#ifdef __LINUX__
    log_error("Linux not support filetrans yet");
#elif __REWORKS_RTP__
    //待传输文件存放位置：用户态使用0x10000000-0x10400000,核态使用0xFFFFFFFFF0000000-0xFFFFFFFFF0400000
    //解析params
    log_info("entry filetrans_cmd_rpc_handler");
    //sleep(1);
    filetrans f;
    filetrans_init(&f);
    filetrans_deserialize(params,&f);
    //f.addr转化为uintptr_t
    uintptr_t addr;
    string_to_uintptr(f.addr,&addr); 
    log_info("filetrans_cmd_rpc_handler : f.addr:%s",f.addr);
    log_info("filetrans_cmd_rpc_handler : addr:%p,file:%s",addr,(char*)addr);
    //使用filetrans的filename、addr、len从指定的共享内存位置获取文件放入文件系统
    cJSON* result = cJSON_CreateObject();
    if (addr && f.len > 0 && f.filename) {
        // 从共享内存读取数据并写入文件系统
        int fd = open(f.filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            log_error("filetrans_cmd_rpc_handler : open file failed");
            ctx->error_code = JRPC_INTERNAL_ERROR;
            ctx->error_message = "open file failed";
            cJSON_AddStringToObject(result, "error", "open file failed");
        } else {
            ssize_t write_bytes = write(fd, (void*)addr, f.len);
            close(fd);
            if (write_bytes != f.len) {
                log_error("filetrans_cmd_rpc_handler : write file failed");
                ctx->error_code = JRPC_INTERNAL_ERROR;
                ctx->error_message = "write file failed";
                cJSON_AddStringToObject(result, "error", "write file failed");
            } else {
                cJSON_AddNumberToObject(result, "exit_code", 0); // 成功
            }
        }
    } else {
        log_error("filetrans_cmd_rpc_handler : Invalid filetrans params");
        ctx->error_code = JRPC_INVALID_PARAMS;
        ctx->error_message = "Invalid filetrans params";
        cJSON_AddStringToObject(result, "error", "Invalid filetrans params");
    }
    filetrans_deinit(&f);
    return result;
#endif
}

node_loader* node_loader_new(char* file_path, abs_comm_connect* rct) {
    node_loader* loader = malloc(sizeof(node_loader));
    if (!loader) return NULL;

    // 初始化节点配置
    node_init(&loader->n);
    if (node_deserialize_file(&loader->n, file_path) != BASE_SUCCESS) {
        log_error("Failed to load node config from: %s", file_path);
        free(loader);
        return NULL;
    }
    
    // 创建注册中心通信
    loader->rc = abs_comm_new(loader->n.id, rct);

    if (!loader->rc) {
        node_deinit(&loader->n);
        free(loader);
        return NULL;
    }

    int ret = abs_comm_sub(loader->rc, loader->n.id, loader, node_loader_deal_center_msg);

    if (BASE_SUCCESS != ret)
    {
        node_deinit(&loader->n);
        free(loader);
        return NULL;
    }

    // 设置节点基础信息
    loader->thread_run_flag = 0;

    loader->rpc_ctx = rpc_context_new(loader->rc);
    pthread_mutex_init(&loader->mutex_log, NULL);
    log_set_lock(log_mutex_lock, &loader->mutex_log);
    log_add_callback(nl_distributed_log_callback, loader, LOG_TRACE);
    // 注册deploy_cmd方法
    rpc_register_method(loader->rpc_ctx, "deploy_cmd", deploy_cmd_rpc_handler, loader);
    // 注册filetrans_cmd方法
    rpc_register_method(loader->rpc_ctx, "filetrans_cmd", filetrans_cmd_rpc_handler, loader);
    return loader;
}

void node_loader_delete(node_loader* nc) {
    if (!nc) return;
    
    abs_comm_delete(nc->rc);
    rpc_context_delete(nc->rpc_ctx);
    
    // 释放节点资源
    node_deinit(&nc->n);
    
    free(nc);
}

void node_loader_run(void* arg) {
    node_loader* loader = arg;
    loader->thread_run_flag = THREAD_RUNING;
    
    while (THREAD_RUNING == loader->thread_run_flag) {
        // 定期上报节点状态
        loader->n.msg = MSG_TYPE_NODE_INFP;
        char* node_str = node_to_str(&loader->n);
        if (node_str) {
            loader->rc->pub_msg(loader->rc, NODE_CENTER, node_str, strlen(node_str));
            free(node_str);
        }
        sleep(1); // 1秒上报一次
    }
}

void node_loader_stop(node_loader* nc) {
    if (!nc) return;
    nc->thread_run_flag = THREAD_STOPING;
    pthread_join(nc->pid, NULL);
}

void node_loader_run_thread(node_loader* nc) {
    if (NULL == nc)
    {
        log_error("nc is null");
        return;
    }
    
    pthread_create(&nc->pid, NULL, (void*)node_loader_run, (void*)nc);
}
