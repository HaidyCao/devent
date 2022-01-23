//
// Created by Haidy on 2021/11/7.
//

#ifndef DOCKET_DOCKET_CONNECT_H
#define DOCKET_DOCKET_CONNECT_H

#include "def.h"
#include "client_context.h"
#include "client_config.h"
#include "event.h"

/**
 * on remote connect
 * @param event event
 * @param ctx server context
 */
void docks_on_remote_connect(DocketEvent *event, RemoteContext *remote_context);

void docks_connect_read(DocketEvent *event, void *ctx);

void docks_send_remote_address(DocketEvent *event, ClientContext *context);

#endif //DOCKET_DOCKET_CONNECT_H
