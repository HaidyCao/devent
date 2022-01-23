//
// Created by Haidy on 2021/9/25.
//

#ifndef DOCKET_READ_H
#define DOCKET_READ_H

#include "docket.h"

/**
 * docket event read
 *
 * @param docket docket
 * @param fd     fd
 */
void docket_on_event_read(DocketEvent *event);

#endif //DOCKET_READ_H
