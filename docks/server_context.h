//
// Created by Haidy on 2021/11/7.
//

#ifndef DOCKET_SERVER_CONTEXT_H
#define DOCKET_SERVER_CONTEXT_H

#include <sys/socket.h>
#include "status.h"
#include "server_config.h"
#include "docks_def.h"
#include "event.h"
#include "def.h"

#define MAX_DOMAIN_LEN 255

typedef void (*docks_custom_connect_remote)(ServerContext *context);

struct server_context {
  ServerConfig *config;
  RemoteContext *remote;
  DocketEvent *event;

  unsigned char method_count;
  unsigned char method;

  char *remote_host;
  uint16_t remote_port;

  char *username;
  char *password;
  docks_custom_connect_remote connect_remote;
  void *custom_ctx;

  char connect_header[SOCKS5_CONN_HEADER_LEN];

  char reply_data[MAX_DOMAIN_LEN + 6];
};

void ServerContext_free(ServerContext *context);

#endif //DOCKET_SERVER_CONTEXT_H
