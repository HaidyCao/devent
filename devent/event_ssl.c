//
// Created by haidy on 2022/3/13.
//

#include "event_ssl.h"
#include "log.h"
#include "buffer_def.h"
#include "utils_internal.h"

#ifdef DEVENT_SSL

DocketEventSSLContext *DocketEventSSLContext_new(SSL_CTX *ssl_ctx) {
  DocketEventSSLContext *event_ssl_context = calloc(1, sizeof(DocketEventSSLContext));
  event_ssl_context->ssl = SSL_new(ssl_ctx);
  event_ssl_context->ssl_handshaking = true;

  BIO *rbio = BIO_new(BIO_s_mem());
  BIO *wbio = BIO_new(BIO_s_mem());

  SSL_set_bio(event_ssl_context->ssl, rbio, wbio);

  event_ssl_context->rbio = rbio;
  event_ssl_context->wbio = wbio;
  return event_ssl_context;
}

void DocketEventSSLContext_free(DocketEventSSLContext *ctx) {
  BIO_free(ctx->rbio);
  BIO_free(ctx->wbio);
  SSL_free(ctx->ssl);

  free(ctx);
}

static void docket_on_ssl_write_transfer(DocketEvent *event, void *ctx) {

}

static void docket_on_ssl_read_transfer(DocketEvent *event, void *ctx) {
  DocketEventSSLContext *event_ssl_context = ctx;

  Buffer *buffer = Docket_buffer_alloc();
  ssize_t len;

  while ((len = DocketEvent_read(event, buffer->data, sizeof(buffer->data))) != -1) {
    BIO_write(event_ssl_context->rbio, buffer->data, (int) len);
  }

  do {
    len = SSL_read(event_ssl_context->ssl, buffer->data, sizeof(buffer->data));


  } while (len > 0);
}

bool DocketEvent_do_handshake(DocketEvent *event, DocketEventSSLContext *event_ssl_context, bool *success) {
  int r = SSL_do_handshake(event_ssl_context->ssl);
  if (r != 1) {
    int err = SSL_get_error(event_ssl_context->ssl, (int) r);
    if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
      LOGE("SSL error: %s", ERR_error_string(err, NULL));
      devent_close_internal(event, DEVENT_CONNECT | DEVENT_ERROR | DEVENT_OPENSSL);
      return false;
    }

    Buffer *buffer = Docket_buffer_alloc();
    int br = BIO_read(event_ssl_context->wbio, buffer->data, sizeof(buffer->data));
    if (br < 0) {
      err = SSL_get_error(event_ssl_context->ssl, br);
      LOGE("SSL error: %s", ERR_error_string(err, NULL));
      Docket_buffer_release(buffer);
      devent_close_internal(event, DEVENT_CONNECT | DEVENT_ERROR | DEVENT_OPENSSL);
      return false;
    }
    DocketEvent_write(event, buffer->data, br);
    Docket_buffer_release(buffer);
    if (success) {
      *success = false;
    }
  } else {
    if (event_ssl_context->ssl_event_cb) {
      event_ssl_context->ssl_event_cb(event, DEVENT_CONNECT_SSL, event_ssl_context->ssl_ctx);
    }
    if (success) {
      *success = true;
    }
  }
  return true;
}

void docket_on_ssl_read(DocketEvent *event, void *ctx) {
  // ssl client handshake
  DocketEventSSLContext *event_ssl_context = ctx;
  LOGD("len = %d", DocketBuffer_length(DocketEvent_get_in_buffer(event)));
  if (event_ssl_context->ssl_handshaking) {
    Buffer *buffer = Docket_buffer_alloc();
    ssize_t r = DocketEvent_read(event, buffer->data, sizeof(buffer->data));

    // write data
    BIO_write(event_ssl_context->rbio, buffer->data, (int) r);
    Docket_buffer_release(buffer);

    // SSL_do_handshake and read data from wbio and write data to remote
    bool success;
    if (!DocketEvent_do_handshake(event, event_ssl_context, &success)) {
      return;
    }

    if (!success) {
      return;
    }

    event_ssl_context->ssl_handshaking = true;
    event->read_cb = docket_on_ssl_read_transfer;
    event->write_cb = docket_on_ssl_write_transfer;

    docket_on_ssl_read_transfer(event, event->ctx);
  } else {
    LOGE("no way here");
    devent_close_internal(event, DEVENT_CONNECT | DEVENT_ERROR | DEVENT_OPENSSL);
  }
}

void docket_on_ssl_write(DocketEvent *event, void *ctx) {
  // TODO ssl server handshake
}

#endif