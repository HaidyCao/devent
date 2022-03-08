//
// Created by haidy on 2022/2/19.
//

#ifndef DOCKET_DEVENT_BUFFER_DEF_H_
#define DOCKET_DEVENT_BUFFER_DEF_H_

#include "docket_def.h"
#include "buffer.h"

#ifdef WIN32
int DocketBuffer_win_send(IO_CONTEXT *io, int flags, struct sockaddr *address, socklen_t socklen);
#endif

#endif //DOCKET_DEVENT_BUFFER_DEF_H_
