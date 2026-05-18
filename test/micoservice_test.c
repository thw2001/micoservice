#include "mico_service.h"
#include "stdlib.h"
#include "cf_log.h"
#include "mstracing.h"
#include "unistd.h"

int main(int argc, char const *argv[])
{
    log_set_level(LOG_DEBUG);
    abs_comm_connect rct = {.ip = "127.0.0.1", .port = 19999};
    mico_service* ms = mico_service_new("test1", &rct);
    service_info_dependencies dp = {.name = "test2"};
    mico_service_add_dependency(ms, &dp);
    
    mico_service* ms2 = mico_service_new("test2", &rct);
    

    mstracing* mst = mstracing_new("test1111");
    mstracing_span_trace(mst);
    mstracing_start_trace(mst);
    mstracing_finsh_trace(mst);
    char* tmp1 = mstracing_span_context_inject(mst);
    char* tmp2 = mstracing_get_span_str(mst);

    mico_service_run_thread(ms);
    mico_service_run_thread(ms2);

    

    sleep(3);

    service_info* si = mico_service_get_instance(ms, "test2", 0);
    
    if (si)
    {
        printf("mico_service_get_instance name: %s, id: %s\n", si->name, si->id);
    }
    
    sleep(5);
    mico_service_stop(ms);
    mico_service_stop(ms2);

    sleep(1);
    mico_service_delete(ms);
    mico_service_delete(ms2);
    return 0;
}
