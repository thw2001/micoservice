#include "mstracing.h"
#include "collector.h"
#include "stdio.h"
#include "stdlib.h"
#include "collector_http_server.h"
#include "cf_log.h"
#include "unistd.h"

int main(int argc, char const *argv[])
{
    /* code */
    log_set_level(1);
    abs_comm_connect rsct = {.ip = "127.0.0.1", .port = 19999};
    collector* c = collector_new(&rsct);

    collector_http_service_thread(c);

    // collector_loop(c);
    while (1)
    {
        sleep(1);
    }
    
    return 0;
}
