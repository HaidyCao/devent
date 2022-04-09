//
// Created by Haidy on 2021/11/7.
//

#ifndef DOCKET_SERVER_H
#define DOCKET_SERVER_H

#include "docket.h"

void docks_on_socks_connect(DocketListener *l, SOCKET fd, struct sockaddr *address, socklen_t address_len,
                            DocketEvent *event,
                            void *ctx);

#endif //DOCKET_SERVER_H
