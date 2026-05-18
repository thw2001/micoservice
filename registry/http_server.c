#if defined(WITH_HTTP_SERVER)
#include "mongoose.h"
#include "cf_log.h"
#include "http_server.h"

#define CORS_HEADERS "Access-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\nAccess-Control-Allow-Headers: *\r\n"

static registry* reg;
static node_center* nc;

static void ev_handler(struct mg_connection *c, int ev, void *ev_data)
{
    if (ev == MG_EV_HTTP_MSG)
    {
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;
        char* http_uri = malloc(sizeof(char) * (hm->uri.len + 1));
        char* http_method = malloc(sizeof(char) * (hm->method.len + 1));
        // char http_uri[60], http_method[60];
        strncpy(http_uri, hm->uri.buf, hm->uri.len);
        http_uri[hm->uri.len] = '\0';
        strncpy(http_method, hm->method.buf, hm->method.len);
        http_method[hm->method.len] = '\0';
        log_debug("%lu %s %lu %s", hm->uri.len, http_uri, hm->method.len, http_method);

        if (strcmp(http_uri, "/api/v1/node/info") == 0)
        {
            if (strcmp(http_method, "GET") == 0)
            {
                char* out = map_node_cluster_serialize_to_str(&nc->map_cluster);
                log_debug("%s", out);
                mg_http_reply(c, 200, CORS_HEADERS, out);
                free(out);
            }
            else
                goto err_method;

        }else if (strcmp(http_uri, "/api/v1/micoservice/info") == 0)
        {
            if (strcmp(http_method, "GET") == 0)
            {
                char *out = map_service_cluster_serialize_to_str(&reg->map_cluster);
                log_debug("%s", out);
                mg_http_reply(c, 200, CORS_HEADERS, out);
                free(out);
            }
            else
                goto err_method;
        }else if (strcmp(http_uri, "/api/v1/node/deploy") == 0)
        {
            if (strcmp(http_method, "GET") == 0)
            {
                char cmd[1024];
                char node_id[64];
                int len = mg_http_get_var(&hm->query, "node_id", node_id, 64);
                if (len < 0)
                    goto err_param;

                len = mg_http_get_var(&hm->query, "cmd", cmd, 1024);
                if (len < 0)
                    goto err_param;

                log_debug("%s %s", node_id, cmd);
                int ret = node_center_deploy_cmd(nc, node_id, cmd);
                mg_http_reply(c, 200, CORS_HEADERS, "{%m:%m}\n", MG_ESC("message"), MG_ESC("1"));
            }
            else
                goto err_method;
        }

        free(http_uri);
        free(http_method);
        return;
    err_method:
        free(http_uri);
        free(http_method);
        mg_http_reply(c, 200, "", "{%m:%m}\n", MG_ESC("error"), MG_ESC("Unsupported Method"));
        return;
    err_param:
        free(http_uri);
        free(http_method);
        mg_http_reply(c, 200, "", "{%m:%m}\n", MG_ESC("error"), MG_ESC("Mismatched Parameter"));
        return;
    }

}

void* http_service_main()
{
    struct mg_mgr mgr;                                               // Declare event manager
    mg_mgr_init(&mgr);                                               // Initialise event manager
    mg_http_listen(&mgr, "http://0.0.0.0:8000", ev_handler, NULL); // Setup listener
    for (;;)
    { // Run an infinite event loop
        mg_mgr_poll(&mgr, 1000);
    }
}

void registry_http_service_thread(registry* r, node_center* n)
{
    reg = r;
    nc = n;
    pthread_t pid;
    pthread_create(&pid, NULL, http_service_main, NULL);
}
#endif
