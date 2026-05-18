#if __REWORKS_RTP__
#include <fcntl.h>
#include <sys/stat.h>
#endif
#include "node_center.h"
#include "stdlib.h"
#include "stdio.h"
#include "cf_log.h"
// #include "unistd.h"
// #include "rpc.h"
// #include "cJSON.h"
// #include "base_types.h"
#include "base_msg_json_tools.h"
#include "uintptr2string.h"

static void deploy_result_callback(void* arg, cJSON* result, int error_code, const char* error_message) {
    node_center* nc = (node_center*)arg;
    if (error_code == 0 && result) {
        int exit_code = -999;
        cJSON* code = cJSON_GetObjectItem(result, "exit_code");
        if (code && cJSON_IsNumber(code)) {
            exit_code = code->valueint;
        }
        log_info("[deploy_result] node_id=%s, exit_code=%d", nc->rs->client_name, exit_code);
    } else {
        log_error("[deploy_result] node_id=%s, error_code=%d, error_msg=%s", nc->rs->client_name, error_code, error_message ? error_message : "");
    }
}

int node_center_deploy_cmd(node_center* nc, char* node_name, char* exec_cmd)
{
    //使用json_to_c_stuct.py根据deploy.json生成的json工具解析json格式的命令
    int ret = 0;
    
    // 创建单个 deploy_config 对象
    deploy_config dc;
    deploy_config_init(&dc);
    dc.msg = MSG_TYPE_NODE_DEPLOY_CMD;
    dc.node_id = strdup(node_name);
    dc.command = strdup(exec_cmd);
    
    // 序列化 deploy_config 对象
    cJSON* cjson = cJSON_CreateObject();
    deploy_config_serialize(&dc, cjson);
    char* param_str = cJSON_PrintUnformatted(cjson);
    cJSON_Delete(cjson);
    
    // 构造rpc请求json字符串
    cJSON* rpc_req = cJSON_CreateObject();
    cJSON_AddStringToObject(rpc_req, "jsonrpc", "2.0");
    cJSON_AddStringToObject(rpc_req, "method", "deploy_cmd");
    cJSON* params_obj = cJSON_Parse(param_str);
    cJSON_AddItemToObject(rpc_req, "params", params_obj);
    char* rpc_str = cJSON_PrintUnformatted(rpc_req);
    cJSON_Delete(rpc_req);
    
    // 通过同步rpc调用
    int error_code;
    char* error_msg;
    cJSON* result = rpc_call_method_sync(
        nc->rpc_ctx,
        node_name,
        rpc_str,
        5000,
        &error_code,
        &error_msg
    );

    if (result) {
        char* result_str = cJSON_PrintUnformatted(result);
        log_debug("Received result: %s", result_str);
        free(result_str);
        cJSON_Delete(result);
    } else {
        log_error("Error occurred, code: %d, message: %s", error_code, error_msg ? error_msg : "Unknown error");
        if (error_msg) {
            free(error_msg);
        }
        ret = error_code;
    }
    
    // 清理资源
    free(param_str);
    free(rpc_str);
    deploy_config_deinit(&dc);
    
    return ret;
}

static void filetrans_result_callback(void* arg, cJSON* result, int error_code, const char* error_message) {
    node_center* nc = (node_center*)arg;
    if (error_code == 0 && result) {
        int exit_code = -999;
        cJSON* code = cJSON_GetObjectItem(result, "exit_code");
        if (code && cJSON_IsNumber(code)) {
            exit_code = code->valueint;
        }
        log_info("[filetrans_result] node_id=%s, exit_code=%d", nc->rs->client_name, exit_code);
    }  else {
        log_error("[filetrans_result] node_id=%s, error_code=%d, error_msg=%s",
                  nc->rs->client_name, error_code, error_message ? error_message : "");
    }
}

//部署者调用的外部文件传输接口
int node_center_filetrans_cmd(node_center* nc,char* filename,char* target_node_id)
{
#ifdef __LINUX__
    log_error("Linux not support filetrans yet");
#elif __REWORKS_RTP__
    //todo: map遍历决定node_id有没有在节点注册，使用一个新函数？
    log_info("entry node_center_filetrans_cmd");
    //文件锁进入临界区
    pthread_mutex_lock(&(nc->file_mutex));//互斥锁并发控制？
    uintptr_t addr = FILE_TRANS_START;
    //搜索文件系统里的文件，将文件从文件系统复制到内存空间指定的位置 ;记录文件在内存中的地址，大小
    //用户态可访问的虚拟地址空间：0x10000000 - 0x20000000 
    //待传输文件存放位置：用户态使用0x10000000-0x10400000,核态使用0xFFFFFFFFF0000000-0xFFFFFFFFF0400000
    //打开文件
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        pthread_mutex_unlock(&(nc->file_mutex));
        log_error("node_center_filetrans_cmd: open");
        return FILLTRANS_ERR_OPEN;
    }
    //获取文件大小
    struct stat st;
    if (fstat(fd, &st) < 0 ) {
        close(fd);
        pthread_mutex_unlock(&(nc->file_mutex));
        log_error("node_center_filetrans_cmd: FSTAT");
        return FILLTRANS_ERR_FSTAT;
    }
    size_t file_size = st.st_size;
    if (file_size > FILE_TRANS_SIZE){//报错超出长度
        close(fd);
        pthread_mutex_unlock(&(nc->file_mutex));
        log_error("node_center_filetrans_cmd: OVERSIZE");
        return FILLTRANS_ERR_OVERSIZE;
    }
    // 读文件内容到共享内存
    ssize_t read_bytes = read(fd, (void*)addr, file_size); // addr为共享内存地址
    close(fd);
    if (read_bytes != file_size) {
        pthread_mutex_unlock(&(nc->file_mutex));
        log_error("node_center_filetrans_cmd: READ");
        return FILLTRANS_ERR_READ;
    }
    log_info("node_center_filetrans_cmd : addr:%p,file:%s",addr,(char*)addr);   
    int len = file_size;
    // 写入完成，释放锁。
    pthread_mutex_unlock(&(nc->file_mutex));
    log_info("node_center_filetrans_cmd: finish shm write");
    //使用json工具（json_to_c_stuct.py根据filetrans.json生成）构造json格式的文件传输命令
    int ret = 0;
    filetrans f;
    filetrans_init(&f);
    f.msg = MSG_TYPE_FILE_TRANS_CMD;
    f.target_node_id = strdup(target_node_id);
    f.filename = strdup(filename);
    //f.filename = strdup("d/test_filetrans.txt");//在核一的node_loader上测试功能
    char *buff = malloc(20);//unintptr_t addr转为字符串存入filetrans结构体
    memset(buff, 0, 20);
    log_info("f.addr: %s ",f.addr);
    uintptr_to_string(addr,buff,sizeof(buff));
    f.addr = buff;
    log_info("f.addr: %s ",f.addr);
    f.len = len;
    log_info("node_center_filetrans_cmd: filetrans_inited");
    // 构造rpc请求json字符串
    cJSON* rpc_req = cJSON_CreateObject();
    cJSON_AddStringToObject(rpc_req, "jsonrpc", "2.0");
    cJSON_AddStringToObject(rpc_req, "method", "filetrans_cmd");
    cJSON * temp = cJSON_CreateObject();
    filetrans_serialize(&f,temp);
    log_info("node_center_filetrans_cmd: filetrans_serialize");
    cJSON_AddItemToObject(rpc_req, "params", temp);
    char* rpc_str = cJSON_PrintUnformatted(rpc_req);
    cJSON_Delete(rpc_req);
    log_info("node_center_filetrans_cmd: begin rpc");
    //rpc通过同步rpc远程调用文件传输方法
    int error_code;
    char* error_msg;
    cJSON* result = rpc_call_method_sync(
        nc->rpc_ctx,
        target_node_id,
        rpc_str,
        5000,
        &error_code,
        &error_msg
    );
    if (result) {
        char* result_str = cJSON_PrintUnformatted(result);
        log_debug("Received result: %s", result_str);
        free(result_str);
        cJSON_Delete(result);
    } else {
        log_error("Error occurred, code: %d, message: %s", error_code, error_msg ? error_msg : "Unknown error");
        if (error_msg) {
            free(error_msg);
        }
        ret = error_code;
    }
    log_info("node_center_filetrans_cmd:end rpc");
    filetrans_deinit(&f);
    free(rpc_str);
    return ret;
#endif
}