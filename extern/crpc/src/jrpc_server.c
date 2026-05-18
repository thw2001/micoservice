#include "server.h"
#include "log.h"
#include "jrpc_server.h"

#undef LOG_TAG
#define LOG_TAG "JrpcServer"

static int JrpcSendResponse(Context *context, char *response) {
	int fd = context->fd;
	// if (conn->debug_level > 1)
    LOGD("JSON Response:\n%s", response);
    WriteBegin(context, 0);
    WriteStr(context, response);
    WriteEnd(context);
	// write(fd, response, strlen(response));
	// write(fd, "\n", 1);
	return 0;
}

static int JrpcSendError(Context *context, int code, char* message, cJSON * id) {
	int return_value = 0;
	cJSON *result_root = cJSON_CreateObject();
	cJSON *error_root = cJSON_CreateObject();
	cJSON_AddNumberToObject(error_root, "code", code);
	cJSON_AddStringToObject(error_root, "message", message);
	cJSON_AddItemToObject(result_root, "error", error_root);
	cJSON_AddItemToObject(result_root, "id", id);
	char * str_result = cJSON_PrintUnformatted(result_root);
	return_value = JrpcSendResponse(context, str_result);
	free(str_result);
	cJSON_Delete(result_root);
	free(message);
	return return_value;
}

static int JrpcSendResult(Context *context, cJSON * result, cJSON * id) {
	int return_value = 0;
	cJSON *result_root = cJSON_CreateObject();
	if (result)
		cJSON_AddItemToObject(result_root, "result", result);
	cJSON_AddItemToObject(result_root, "id", id);

	char * str_result = cJSON_PrintUnformatted(result_root);
	return_value = JrpcSendResponse(context, str_result);
	free(str_result);
	cJSON_Delete(result_root);
	return return_value;
}

static int JrpcInvokeProcedure(JrpcServer *server, Context *context, char *name, cJSON *params, cJSON *id) {
	cJSON *returned = NULL;
	int procedure_found = 0;
	JrpcContext ctx;
	ctx.error_code = 0;
	ctx.error_message = NULL;
	int i = server->procedure_count;
	while (i--) {
		if (!strcmp(server->procedures[i].name, name)) {
			procedure_found = 1;
			ctx.data = server->procedures[i].data;
			returned = server->procedures[i].function(&ctx, params, id);
			break;
		}
	}
	if (!procedure_found)
		return JrpcSendError(context, JRPC_METHOD_NOT_FOUND,
				strdup("Method not found."), id);
	else {
		if (ctx.error_code)
			return JrpcSendError(context, ctx.error_code, ctx.error_message, id);
		else
			return JrpcSendResult(context, returned, id);
	}
}

static int JrpcEvalRequest(JrpcServer *server, Context *context, cJSON *root) {
	cJSON *method, *params, *id;
	method = cJSON_GetObjectItem(root, "method");
	if (method != NULL && method->type == cJSON_String) {
		params = cJSON_GetObjectItem(root, "params");
		if (params == NULL|| params->type == cJSON_Array
		|| params->type == cJSON_Object) {
			id = cJSON_GetObjectItem(root, "id");
			if (id == NULL|| id->type == cJSON_String
			|| id->type == cJSON_Number) {
			//We have to copy ID because using it on the reply and deleting the response Object will also delete ID
				cJSON * id_copy = NULL;
				if (id != NULL)
					id_copy =
							(id->type == cJSON_String) ? cJSON_CreateString(
									id->valuestring) :
									cJSON_CreateNumber(id->valueint);
				// if (server->debug_level)
					LOGD("Method Invoked: %s", method->valuestring);
				return JrpcInvokeProcedure(server, context, method->valuestring,
						params, id_copy);
			}
		}
	}
	JrpcSendError(context, JRPC_INVALID_REQUEST,
			strdup("The JSON sent is not a valid Request object."), NULL);
	return -1;
}

int JrpcOnTransact(void *server, Context *context)
{
    char tmp[128] = {0};
    int ret = ReadStr(context, tmp, 128);
    LOGD("%s", tmp);
    if (ret != 0) {
        JrpcSendError(context, JRPC_PARSE_ERROR,
                   strdup(
                       "Parse error. JSON was too long."),
                   NULL);
        return -1;
    }
    ret = 0;
    cJSON *root;
    const char *end_ptr = NULL;

    if ((root = cJSON_ParseWithOpts(tmp, &end_ptr, cJSON_False)) != NULL)
    {
        if (root->type == cJSON_Object)
        {
            JrpcEvalRequest(server, context, root);
        }

        cJSON_Delete(root);
    }
    else
    {

        printf("INVALID JSON Received:\n---\n%s\n---\n",
               tmp);

        JrpcSendError(context, JRPC_PARSE_ERROR,
                   strdup(
                       "Parse error. Invalid JSON was received by the server."),
                   NULL);
        return CRPC_FAILED;
    }

    return 0;
}

int JrpcRegisterProcedure(JrpcServer *server, JrpcFunc function_pointer, char *name, void * data) {
	int i = server->procedure_count++;
	if (!server->procedures)
		server->procedures = malloc(sizeof(JrpcProcedure));
	else {
		JrpcProcedure * ptr = realloc(server->procedures,
				sizeof(JrpcProcedure) * server->procedure_count);
		if (!ptr)
			return -1;
		server->procedures = ptr;

	}
	if ((server->procedures[i].name = strdup(name)) == NULL)
		return -1;
	server->procedures[i].function = function_pointer;
	server->procedures[i].data = data;
	return 0;
}

static void JrpcProcedureDestroy(JrpcProcedure *procedure){
	if (procedure->name){
		free(procedure->name);
		procedure->name = NULL;
	}
	if (procedure->data){
		free(procedure->data);
		procedure->data = NULL;
	}
}

int JrpcDeregisterProcedure(JrpcServer *server, char *name) {
	/* Search the procedure to deregister */
	int i;
	int found = 0;
	if (server->procedures){
		for (i = 0; i < server->procedure_count; i++){
			if (found)
				server->procedures[i-1] = server->procedures[i];
			else if(!strcmp(name, server->procedures[i].name)){
				found = 1;
				JrpcProcedureDestroy( &(server->procedures[i]) );
			}
		}
		if (found){
			server->procedure_count--;
			if (server->procedure_count){
				JrpcProcedure * ptr = realloc(server->procedures,
					sizeof(JrpcProcedure) * server->procedure_count);
				if (!ptr){
					perror("realloc");
					return -1;
				}
				server->procedures = ptr;
			}else{
				server->procedures = NULL;
			}
		}
	} else {
		fprintf(stderr, "server : procedure '%s' not found\n", name);
		return -1;
	}
	return 0;
}

JrpcServer* CreateJrpcServer(const char *ip, uint16_t port)
{
	JrpcServer* server = malloc(sizeof(JrpcServer));
	server->procedure_count = 0;
    server->server = CreateRpcServer(ip, port);
	server->server->arg = server;
    if (server->server)
    {
        server->server->serverOnTransact = JrpcOnTransact;
        server->server->serverOnCallbackTransact = NULL;
        server->server->serverEndCallbackTransact = NULL;
        return server;
    }
    
    return NULL;
}

int RunJrpcLoop(JrpcServer *server)
{
    return RunRpcLoop(server->server);
}

void ReleaseJrpcServer(JrpcServer *server)
{
    int i;
	for (i = 0; i < server->procedure_count; i++){
		JrpcProcedureDestroy( &(server->procedures[i]) );
	}
	free(server->procedures);
    ReleaseRpcServer(server->server);
	free(server);
}
