//
// Created by haidy on 2022/3/21.
//

#ifndef DOCKET_DEVENT_FILE_EVENT_H_
#define DOCKET_DEVENT_FILE_EVENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "event.h"

DocketEvent *Docket_create_stdin_event(Docket *docket);

DocketEvent *Docket_create_file_event(Docket *docket, int fd);

#ifdef __cplusplus
}
#endif

#endif //DOCKET_DEVENT_FILE_EVENT_H_
