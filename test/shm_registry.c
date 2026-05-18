#include "registry.h"
#include "node_center.h"
#include "stdlib.h"
#include "cf_log.h"
#include "http_server.h"
#include "unistd.h"
#include "pubsub.h"
#include "om.h"

static char test_deploy_cmd[] = "{\"msg\":0,\"config\":[{\"msg\":0,\"node_id\":\"node1\",\"command\":\"/usr/bin/ls\"}]}";

int main(int argc, char const *argv[])
{
    log_set_level(LOG_DEBUG);
    abs_comm_connect rsct = {.ip = "127.0.0.1", .port = 19999};
    registry* registry1 = registry_new(&rsct);
    node_center* ns = node_center_new(&rsct);
    registry_run_thread(registry1);

    node_center_run_thread(ns);
    registry_http_service_thread(registry1, ns);

    while (1)
    {
        // node_center_deploy_cmd(ns, test_deploy_cmd);
        // char* tmp = map_service_cluster_serialize_to_str(&registry1->map_cluster);
        // char* tmp2 = map_node_cluster_serialize_to_str(&ns->map_cluster);
        // log_debug("%s", tmp2);
        sleep(10);
        // shm_print_stats();
    }
    
    registry_stop(registry1);
    registry_delete(registry1);
    return 0;
}
