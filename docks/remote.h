//
// Created by Haidy on 2021/11/13.
//

#ifndef DOCKET_REMOTE_H
#define DOCKET_REMOTE_H

#include "event.h"
#include "remote_context.h"

void docket_remote_connect(DocketEvent *event, unsigned int what, void *ctx);

#endif //DOCKET_REMOTE_H
