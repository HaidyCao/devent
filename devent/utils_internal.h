//
// Created by Haidy on 2021/12/5.
//

#ifndef DOCKET_DEVENT_UTILS_INTERNAL_H
#define DOCKET_DEVENT_UTILS_INTERNAL_H

#include "utils.h"

/**
 * socket add flags
 * @param fd fd
 * @param flags flags
 * @return -1 failed
 */
int devent_turn_on_flags(int fd, int flags);

int devent_update_events(int kq, int fd, int events, int mod);

#include "event.h"

void devent_close_internal(DocketEvent *event, int what);

bool errno_is_EAGAIN(int eno);

#ifdef DEVENT_SSL
bool devent_do_ssl_handshake(DocketEvent *event);
#endif

#endif //DOCKET_DEVENT_UTILS_INTERNAL_H
