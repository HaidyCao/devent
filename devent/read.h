//
// Created by Haidy on 2021/9/25.
//

#ifndef DOCKET_READ_H
#define DOCKET_READ_H

#include "docket_def.h"
#include "event.h"

#ifdef WIN32
void docket_on_handle_read_data(IO_CONTEXT *io, DWORD size, DocketEvent *event);
#else

/**
 * docket event read
 *
 * @param docket docket
 * @param fd     fd
 */
void docket_on_event_read(DocketEvent *event);
#endif

#endif //DOCKET_READ_H
