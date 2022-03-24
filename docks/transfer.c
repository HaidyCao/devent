//
// Created by Haidy on 2021/11/28.
//

#include "transfer.h"
#include "remote_context.h"
#include "log.h"

static void remote_disconnect_write(DocketEvent *event, void *ctx) {
  LOGD("");
  RemoteContext *remote = ctx;
  if (DocketBuffer_length(DocketEvent_get_out_buffer(event)) == 0) {
    DocketEvent_free(event);
    RemoteContext_free(remote);
  }
}

static void server_disconnect_write(DocketEvent *event, void *ctx) {
  LOGD("");
  ServerContext *context = ctx;

  if (DocketBuffer_length(DocketEvent_get_out_buffer(event)) == 0) {
    LOGI("disconnect");
    DocketEvent_free(event);
    ServerContext_free(context);
  }
}

static void docket_server_disconnect_finally(DocketEvent *event, int what, void *ctx) {
  ServerContext *server = ctx;
  ServerContext_free(server);
}

void docket_server_disconnect_event(DocketEvent *event, int what, void *ctx) {
  LOGD("");
  ServerContext *server = ctx;
  RemoteContext *remote = server->remote;
  if ((what & DEVENT_ERROR) || (what & DEVENT_EOF)) {
    DocketEvent_free(event);
    ServerContext_free(server);
    remote->server = NULL;

    if (DocketBuffer_length(DocketEvent_get_out_buffer(remote->event)) == 0) {
      DocketEvent_free(remote->event);
      RemoteContext_free(remote);
    } else {
      DocketEvent_set_cb(remote->event, NULL, remote_disconnect_write, docket_server_disconnect_finally, remote);
    }
  }
}

static void docket_remote_disconnect_finally(DocketEvent *event, int what, void *ctx) {
  RemoteContext *remote = ctx;
  RemoteContext_free(remote);
}

void docket_remote_disconnect_event(DocketEvent *event, int what, void *ctx) {
  LOGD("");
  RemoteContext *remote = ctx;
  ServerContext *server = remote->server;
  if ((what & DEVENT_ERROR) || (what & DEVENT_EOF)) {
    DocketEvent_free(event);
    RemoteContext_free(remote);
    server->remote = NULL;

    if (DocketBuffer_length(DocketEvent_get_out_buffer(server->event)) == 0) {
      DocketEvent_free(server->event);
      ServerContext_free(server);
    } else {
      DocketEvent_set_cb(server->event, NULL, server_disconnect_write, docket_remote_disconnect_finally, server);
    }
  }
}

void docket_remote_read(DocketEvent *event, void *ctx) {
  RemoteContext *remote = ctx;
  ServerContext *server = remote->server;

  DocketEvent_write_buffer_force(server->event, DocketEvent_get_in_buffer(event));
}

void docket_server_read(DocketEvent *event, void *ctx) {
  ServerContext *server = ctx;
  RemoteContext *remote = server->remote;

  DocketEvent_write_buffer_force(remote->event, DocketEvent_get_in_buffer(event));
}