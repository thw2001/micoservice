#include "mico_service.h"
#include "stdlib.h"
#include "cf_log.h"
#include "mstracing.h"
#include "snow_flake.h"
#include "unistd.h"

int lbSubOnMessage(void* arg, void* msg, uint32_t len)
{
    int *add = (int*)msg;
    log_info("msg: %d", *add);
}

int main(int argc, char const *argv[])
{
    log_set_level(LOG_DEBUG);
    abs_comm_connect rct = {.ip = "127.0.0.1", .port = 19999};

    mico_service* ms2 = mico_service_new("lb_recv", &rct);


    char* rand_id = id_generate_string();

    int ret = mico_service_sub(ms2, rand_id, NULL, lbSubOnMessage);

    log_info("%d", ret);

    mico_service_set_endpoint(ms2, rand_id);

    mico_service_run_thread(ms2);

    while (1)
    {
        sleep(1);
    }
    

    return 0;
}
