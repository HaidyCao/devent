//
// Created by Haidy on 2021/12/21.
//

#include "client_context.h"

void ClientContext_free(ClientContext *context) {
  if (context == NULL) {
    return;
  }

  free(context->bind_host);
  free(context->remote_host);

  free(context);
}