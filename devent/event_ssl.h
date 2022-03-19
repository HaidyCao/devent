//
// Created by haidy on 2022/3/13.
//

#ifndef DOCKET_DEVENT_EVENT_SSL_H_
#define DOCKET_DEVENT_EVENT_SSL_H_
#ifdef DEVENT_SSL

#include "event.h"
#include "event_def.h"

typedef struct docket_event_ssl_ctx {
  docket_event_ssl_callback ssl_event_cb;

  /**
   * ssl read callback
   */
  docket_event_ssl_read_callback ssl_read_cb;

  /**
   * ssl write callback
   */
  docket_event_ssl_write_callback ssl_write_cb;

  /**
  * ssl ctx
  */
  void *ssl_ctx;

  SSL *ssl;
  BIO *rbio;
  BIO *wbio;

  DocketListener *listener;

  // SSL in buffer and out buffer
  DocketBuffer *ssl_in_buffer;
  DocketBuffer *ssl_out_buffer;

  bool ssl_handshaking;
} DocketEventSSLContext;

struct docket_event_ssl {
  DocketEvent *event;
};

DocketEventSSL *DocketEventSSL_new(DocketEvent *event);

void DocketEventSSL_free(DocketEventSSL *event_ssl);

DocketEventSSLContext *DocketEventSSLContext_new(SSL_CTX *ssl_ctx);

void DocketEventSSLContext_free(DocketEventSSLContext *ctx);

bool DocketEvent_do_handshake(DocketEvent *event, DocketEventSSLContext *event_ssl_context, bool *success);

void docket_on_ssl_read(DocketEvent *event, void *ctx);

void docket_on_ssl_write(DocketEvent *event, void *ctx);

void docket_on_ssl_event(DocketEvent *event, int what, void *ctx);

#endif
#endif //DOCKET_DEVENT_EVENT_SSL_H_
