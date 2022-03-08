//
// Created by Haidy on 2021/11/6.
//

#ifndef DEVENT_WRITE_H
#define DEVENT_WRITE_H

#ifndef WIN32
#include <sys/socket.h>
#else
#include <WS2tcpip.h>
#endif
#include "def.h"
#include "win_def.h"

void devent_write_data(DocketEvent *event, DocketBuffer *buffer);

/**
 * on event write
 * @param docket docket
 * @param fd event fd
 */
void docket_on_event_write(DocketEvent *event);

#endif //DEVENT_WRITE_H
