//
// Created by Haidy on 2021/12/21.
//

#ifndef DOCKET_DOCKS_CLIENT_CONTEXT_H
#define DOCKET_DOCKS_CLIENT_CONTEXT_H

#include "def.h"
#include "client_config.h"
#include "event.h"

typedef void (*docks_client_on_server_connect)(DocketEvent *event, ClientContext *context);
typedef void (*docks_client_handshake_failed)(int what, void *ctx);

struct client_context {
  ClientConfig *config;
  DocketEvent *event;

  char *remote_host;
  unsigned short remote_port;

  char *bind_host;
  unsigned short bind_port;
  docks_client_on_server_connect on_server_connect;
  docks_client_handshake_failed handshake_failed;

  void *ctx;
};

void ClientContext_free(ClientContext *context);

#endif //DOCKET_DOCKS_CLIENT_CONTEXT_H
