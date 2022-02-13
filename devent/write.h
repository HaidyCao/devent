//
// Created by Haidy on 2021/11/6.
//

#ifndef DEVENT_WRITE_H
#define DEVENT_WRITE_H

#include <sys/socket.h>
#include "def.h"

void devent_write_data(DocketEvent *event, DocketBuffer *buffer, struct sockaddr *address, socklen_t socklen);

/**
 * on event write
 * @param docket docket
 * @param fd event fd
 */
void docket_on_event_write(DocketEvent *event);

#endif //DEVENT_WRITE_H
