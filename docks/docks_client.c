//
// Created by Haidy on 2021/12/20.
//
#include "docket.h"
#include "client_config.h"
#include "clib.h"
#include "log.h"
#include "listener.h"
#include "def.h"
#include "server_context.h"
#include "method.h"
#include "connect.h"
#include "docks_connect.h"
#include "remote_context.h"

#include <string.h>
#include <stdlib.h>

static void remote_event(DocketEvent *event, int what, void *ctx) {
  if (DEVENT_IS_ERROR_OR_EOF(what)) {
    RemoteContext *remote_context = ctx;
    ServerContext *server_context = remote_context->server;

    DocketEvent_free(event);
    RemoteContext_free(remote_context);

    DocketEvent_free(server_context->event);
    ServerContext_free(server_context);
  }
}

static void server_event(DocketEvent *event, int what, void *ctx) {
  if (DEVENT_IS_ERROR_OR_EOF(what)) {
    ServerContext *server_context = ctx;
    RemoteContext *remote_context = server_context->remote;

    DocketEvent_free(event);
    ServerContext_free(server_context);

    DocketEvent_free(remote_context->event);
    RemoteContext_free(remote_context);
  }
}

static void on_connect(DocketEvent *event, ClientContext *client_context) {
  // free client context and transfer data between client and server
  ServerContext *server_context = client_context->ctx;
  ClientContext_free(client_context);

  RemoteContext *remote_context = calloc(1, sizeof(RemoteContext));
  remote_context->event = event;
  remote_context->server = server_context;

  server_context->remote = remote_context;

  DocketEvent_set_event_cb(remote_context->event, remote_event, remote_context);
  DocketEvent_set_event_cb(server_context->event, server_event, server_context);

  docks_on_remote_connect(event, remote_context);
}

static void on_connect_to_server(DocketEvent *event, int what, void *ctx) {
  if ((what & DEVENT_ERROR) || (what & DEVENT_EOF)) {
    LOGI("what = %x", what);
    if (event) {
      DocketEvent_free(event);
    }
    return;
  }

  if (what & DEVENT_CONNECT) {
    LOGD("%X", ((ClientContext *) ctx)->on_server_connect);

    docks_write_methods(ctx);
  }
}

static void connect_remote(ServerContext *server_context) {
  LOGD("start connect remote");
  ClientConfig *client_config = server_context->custom_ctx;

  DocketEvent *event;
  if (client_config->ssl) {
    event = DocketEvent_connect_hostname_ssl(DocketEvent_getDocket(server_context->event),
                                             -1,
                                             client_config->remote_host,
                                             client_config->remote_port);
  } else {
    event = DocketEvent_connect_hostname(DocketEvent_getDocket(server_context->event),
                                         -1,
                                         client_config->remote_host,
                                         client_config->remote_port);
  }

  if (event == NULL) {
    DocketEvent_free(server_context->event);
    ServerContext_free(server_context);
    return;
  }

  ClientContext *client_context = calloc(1, sizeof(ClientContext));
  client_context->config = client_config;
  client_context->remote_host = strdup(server_context->remote_host);
  client_context->remote_port = server_context->remote_port;
  client_context->on_server_connect = on_connect;

  client_context->event = event;
  client_context->ctx = server_context;

  DocketEvent_set_event_cb(event, on_connect_to_server, client_context);
}

static void server_event_cb(DocketEvent *ev, int what, void *ctx) {
  // TODO
  if (what & DEVENT_ERROR) {

  }
}

static void docks_connect_cb(DocketListener *l, int fd,
                             struct sockaddr *address,
                             socklen_t address_len, DocketEvent *event,
                             void *ctx) {
  ClientConfig *client_config = ctx;
  ServerConfig *server_config = calloc(1, sizeof(ServerConfig));
  server_config->ssl = false;
  server_config->users = NULL;
  server_config->bind_type = SOCKS5_ATYPE_IPV4;

  server_config->bind_address = malloc(IPV4_LEN);
  n_write_uint32_t_to_data(server_config->bind_address, INADDR_LOOPBACK, 0);
  server_config->bind_address_length = IPV4_LEN;

  LOGD("on connect: fd = %d, address = %s", fd, sockaddr_to_string(address, NULL, 0));
  // create socks context
  ServerContext *context = calloc(1, sizeof(ServerContext));
  context->config = server_config;
  context->event = event;
  context->connect_remote = connect_remote;
  context->custom_ctx = client_config;

  DocketEvent_set_cb(event, docks_on_read_methods, NULL, server_event_cb, context);
}

int main(int argc, char **args) {
  ClientConfig config;
  bzero(&config, sizeof(config));
  if (init_client_config(&config, argc, args) == -1) {
    return -1;
  }

  // check remote address
  struct sockaddr_storage local;
  socklen_t socklen = sizeof(local);
  if (parse_address(config.address, (struct sockaddr *) &local, &socklen) == -1) {
    LOGE("parse address failed: %s", config.address);
    return -1;
  }

  Docket *docket = Docket_new();
  Docket_set_dns_server(docket, "114.114.114.114");

  Docket_create_listener(docket, docks_connect_cb, -1, (struct sockaddr *) &local, socklen, &config);
  Docket_loop(docket);
}
