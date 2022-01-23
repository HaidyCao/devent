//
// Created by Haidy on 2021/11/7.
//

#ifndef DOCKET_AUTH_H
#define DOCKET_AUTH_H

#include "event.h"

void docks_on_auth_read(DocketEvent *event, void *ctx);

void docks_on_read_UP(DocketEvent *event, void *ctx);

#endif //DOCKET_AUTH_H
