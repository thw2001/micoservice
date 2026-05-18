#include "mico_service.h"
#include "stdlib.h"
#include "cf_log.h"
#include "mstracing.h"
#include "unistd.h"

int main(int argc, char const *argv[])
{
    log_set_level(LOG_DEBUG);
    abs_comm_connect rct = {.ip = "127.0.0.1", .port = 19999};
    mico_service* ms = mico_service_new("lb_send", &rct);
    service_info_dependencies dp = {.name = "lb_recv"};
    mico_service_add_dependency(ms, &dp);
    mico_service_node(ms, "node.json");
    
    mico_service_run_thread(ms);

    int add = 0;

    while (1)
    {
        service_info* si = mico_service_get_instance(ms, "lb_recv", 1);
        if (si)
        {
            mico_service_pub(ms, si->endpoint, (uint8_t*)&add, sizeof(add));
            add++;
        }
        sleep(1);
    }
    
    return 0;
}
