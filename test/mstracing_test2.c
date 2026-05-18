#include "mico_service.h"
#include "stdlib.h"
#include "cf_log.h"
#include "mstracing.h"
#include "agent.h"
#include "snow_flake.h"
#include "unistd.h"

agent* a;
int test2SubOnMessage(void *arg, void* payload, uint32_t len)
{
    log_debug("payload:%s", payload);
    mstracing* mst = arg;
    mstracing_span_context_extract(mst, payload);
    // mstracing_span_trace(mst);
    mstracing_start_trace(mst);
    mstracing_finsh_trace(mst);
    
    char* tmp1 = mstracing_span_context_inject(mst);
    char* tmp2 = mstracing_get_span_str(mst);
    log_debug("%s, \n %s", tmp1, tmp2);
    agent_sendto_collector(a, tmp2, strlen(tmp2));
    free(tmp1);
    free(tmp2);
    mstracing_clear_current_tracing(mst);
}

int main(int argc, char const *argv[])
{
    log_set_level(LOG_DEBUG);
    abs_comm_connect rct = {.ip = "127.0.0.1", .port = 19999};

    mico_service* ms2 = mico_service_new("test2", &rct);
    mstracing* mst = mstracing_new("test2222");
    a = agent_new(&rct);

    char* rand_id = id_generate_string();

    mico_service_sub(ms2, rand_id, mst, test2SubOnMessage);


    mico_service_set_endpoint(ms2, rand_id);

    mico_service_run_thread(ms2);


    sleep(8);

    mico_service_stop(ms2);
    mstracing_delete(mst);
    sleep(1);
    mico_service_delete(ms2);
    return 0;
}
