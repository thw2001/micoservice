#include "mico_service.h"
#include "stdlib.h"
#include "cf_log.h"
#include "mstracing.h"
#include "agent.h"
#include "unistd.h"

int main(int argc, char const *argv[])
{
    log_set_level(LOG_DEBUG);
    abs_comm_connect rct = {.ip = "127.0.0.1", .port = 19999};
    // test_create();
    // return;

    agent* a = agent_new(&rct);

    mico_service* ms = mico_service_new("test1", &rct);

    service_info_dependencies dp = {.name = "test2"};
    mico_service_add_dependency(ms, &dp);

    // mico_service_set_endpoint(ms, "yewu1");

    mico_service_run_thread(ms);
    mstracing* mst = mstracing_new("test1111");
    while (1)
    {
        
        mstracing_span_trace(mst);
        mstracing_start_trace(mst);
        usleep(500000);
        mstracing_finsh_trace(mst);
        char* tmp1 = mstracing_span_context_inject(mst);
        char* tmp2 = mstracing_get_span_str(mst);
        // log_debug("%s, \n %s", tmp1, tmp2);
        agent_sendto_collector(a, tmp2, strlen(tmp2));
        service_info* si = mico_service_get_instance(ms, "test2", 1);
        if (si)
        {
            mico_service_pub(ms, si->endpoint, tmp1, strlen(tmp1));
        }
        
        free(tmp1);
        free(tmp2);
        mstracing_clear_current_tracing(mst);
        
        sleep(1);
    }

    mstracing_delete(mst);

    sleep(3);

    // service_info* si = mico_service_get_instance(ms, "test2", 0);
    
    // if (si)
    // {
    //     printf("mico_service_get_instance name: %s, id: %s\n", si->name, si->id);
    // }
    
    sleep(5);
    mico_service_stop(ms);

    sleep(1);
    mico_service_delete(ms);
    return 0;
}
