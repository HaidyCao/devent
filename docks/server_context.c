//
// Created by Haidy on 2021/11/7.
//

#include <stdlib.h>
#include "server_context.h"

void ServerContext_free(ServerContext *context) {
    free(context->username);
    free(context->password);
    free(context->remote_host);
}