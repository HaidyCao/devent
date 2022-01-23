//
// Created by Haidy on 2021/11/7.
//

#ifndef DOCKET_METHOD_H
#define DOCKET_METHOD_H

#include "client_context.h"

/**
 * write methods to server
 * @param context client context
 */
void docks_write_methods(ClientContext *context);

/**
 * docks server read methods from client
 * @param event event
 * @param ctx ctx
 */
void docks_on_read_methods(DocketEvent *event, void *ctx);

#endif //DOCKET_METHOD_H
