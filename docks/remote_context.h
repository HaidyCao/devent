//
// Created by Haidy on 2021/11/13.
//

#ifndef DOCKET_REMOTE_CONTEXT_H
#define DOCKET_REMOTE_CONTEXT_H

#include "server_context.h"
#include "def.h"

struct remote_context {
    DocketEvent *event;
    ServerContext *server;
};

void RemoteContext_free(RemoteContext *context);

#endif //DOCKET_REMOTE_CONTEXT_H
