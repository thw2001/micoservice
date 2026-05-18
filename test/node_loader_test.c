#include "node_loader.h"
#include "stdlib.h"
#include "cf_log.h"
#include "unistd.h"

int main(int argc, char const *argv[])
{
    abs_comm_connect rct = {.ip = "127.0.0.1", .port = 19999};
    node_loader* nl = node_loader_new("node.json", &rct);
    node_loader_run_thread(nl);

    while (1)
    {
        sleep(1);
    }
    

    return 0;
}
