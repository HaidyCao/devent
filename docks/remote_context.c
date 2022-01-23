//
// Created by Haidy on 2021/11/13.
//

#include "remote_context.h"

void RemoteContext_free(RemoteContext *context) {
    free(context);
}