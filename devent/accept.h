//
// Created by Haidy on 2021/10/31.
//

#ifndef DEVENT_ACCEPT_H
#define DEVENT_ACCEPT_H

#include "docket.h"

/**
 * on listener accept
 * @param docket docket
 * @param fd listener fd
 * @return -1 failed
 */
int docket_accept(Docket *docket, int fd);

#endif //DEVENT_ACCEPT_H
