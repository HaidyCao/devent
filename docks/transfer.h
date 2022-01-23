//
// Created by Haidy on 2021/11/28.
//

#ifndef DOCKET_TRANSFER_H
#define DOCKET_TRANSFER_H

#include "event.h"

void docket_server_disconnect_event(DocketEvent *event, int what, void *ctx);

void docket_remote_disconnect_event(DocketEvent *event, int what, void *ctx);

void docket_remote_read(DocketEvent *event, void *ctx);

void docket_server_read(DocketEvent *event, void *ctx);

#endif //DOCKET_TRANSFER_H
