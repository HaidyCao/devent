//
// Created by haidy on 2022/3/13.
//

#ifndef DOCKET_DEVENT_EVENT_SSL_H_
#define DOCKET_DEVENT_EVENT_SSL_H_

#include "event.h"
#include "event_def.h"

typedef struct docket_event_ssl_ctx {
  docket_event_callback ssl_event_cb;

  /**
   * ssl read callback
   */
  docket_event_read_callback ssl_read_cb;

  /**
   * ssl write callback
   */
  docket_event_write_callback ssl_write_cb;

  /**
  * ssl ctx
  */
  void *ssl_ctx;

  SSL *ssl;
  BIO *rbio;
  BIO *wbio;

  DocketListener *listener;

  // TODO SSL in buffer and out buffer

  bool ssl_handshaking;
} DocketEventSSLContext;

DocketEventSSLContext *DocketEventSSLContext_new(SSL_CTX *ssl_ctx);

void DocketEventSSLContext_free(DocketEventSSLContext *ctx);

bool DocketEvent_do_handshake(DocketEvent *event, DocketEventSSLContext *event_ssl_context, bool *success);

void docket_on_ssl_read(DocketEvent *event, void *ctx);

void docket_on_ssl_write(DocketEvent *event, void *ctx);

#endif //DOCKET_DEVENT_EVENT_SSL_H_
