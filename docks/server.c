//
// Created by Haidy on 2021/11/7.
//
#include "clib.h"
#include "log.h"
#include "server.h"
#include "server_context.h"
#include "status.h"
#include "event.h"
#include "method.h"
#include "docket.h"

static void server_event_cb(DocketEvent *ev, int what, void *ctx) {

}

//int docket_cert_verify_callback(X509_STORE_CTX *store_ctx, void *ctx) {
//  //
//  return 1;
//}

void docks_on_socks_connect(DocketListener *l, SOCKET fd, struct sockaddr *address, socklen_t address_len,
                            DocketEvent *event,
                            void *ctx) {
  ServerConfig *config = ctx;
  LOGD("on connect: fd = %d, address = %s", fd, sockaddr_to_string(address, NULL, 0));
  // create socks context
  ServerContext *context = calloc(1, sizeof(ServerContext));
  context->config = config;
  context->event = event;

  DocketEvent_set_cb(event, docks_on_read_methods, NULL, server_event_cb, context);
}