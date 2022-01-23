//
// Created by Haidy on 2021/11/13.
//

#include "remote.h"
#include "log.h"

void docket_remote_connect(DocketEvent *event, unsigned int what, void *ctx) {
    if (what & DEVENT_CONNECT && !(what & DEVENT_ERROR)) {
        LOGI("connect to remote: ");
        // TODO
        return;
    }
}