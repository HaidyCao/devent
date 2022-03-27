//
// Created by Haidy on 2021/9/25.
//
#include <stdlib.h>
#include <stdbool.h>
#ifdef WIN32

#else
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#include "buffer.h"
#include "write.h"
#include "utils_internal.h"
#include "docket.h"
#include "event_def.h"
#include "log.h"
#include "dns.h"
#include "event_ssl.h"
#include "buffer_def.h"

DocketEvent *DocketEvent_new(Docket *docket, SOCKET fd, void *ctx) {
  LOGD("fd = %d", fd);
  DocketEvent *event = calloc(1, sizeof(DocketEvent));
  event->docket = docket;
  event->fd = fd;
  event->ev = DEVENT_READ_WRITE;
  event->connected = true;
  event->ctx = ctx;
  event->remote_address = NULL;
  event->remote_address_len = 0;
  event->out_buffer = DocketBuffer_new();
  event->in_buffer = DocketBuffer_new();

#ifdef DEVENT_SSL
  event->ssl = false;
#endif

  return event;
}

Docket *DocketEvent_getDocket(DocketEvent *event) {
  if (event)
    return event->docket;
  return NULL;
}

DocketBuffer *DocketEvent_get_in_buffer(DocketEvent *event) {
  if (event) {
#ifdef DEVENT_SSL
    if (event->ssl) {
      DocketEventSSLContext *ssl_context = event->ctx;
      return ssl_context->ssl_in_buffer;
    }
#endif
    return event->in_buffer;
  }
  return NULL;
}

DocketBuffer *DocketEvent_get_out_buffer(DocketEvent *event) {
  if (event) {
#ifdef DEVENT_SSL
    if (event->ssl) {
      DocketEventSSLContext *ssl_context = event->ctx;
      return ssl_context->ssl_out_buffer;
    }
#endif
    return event->out_buffer;
  }
  return NULL;
}

void DocketEvent_set_read_cb(DocketEvent *event, docket_event_read_callback cb, void *ctx) {
#ifdef DEVENT_SSL
  if (event->ssl) {
    DocketEventSSLContext *ssl_context = event->ctx;
    ssl_context->ssl_read_cb = cb;
    ssl_context->ssl_ctx = ctx;
    return;
  }
#endif
  event->read_cb = cb;
  event->ctx = ctx;
}

void DocketEvent_set_write_cb(DocketEvent *event, docket_event_write_callback cb, void *ctx) {
#ifdef DEVENT_SSL
  if (event->ssl) {
    DocketEventSSLContext *ssl_context = event->ctx;
    ssl_context->ssl_write_cb = cb;
    ssl_context->ssl_ctx = ctx;
    return;
  }
#endif
  event->write_cb = cb;
  event->ctx = ctx;
}

void DocketEvent_set_event_cb(DocketEvent *event, docket_event_callback cb, void *ctx) {
#ifdef DEVENT_SSL
  if (event->ssl) {
    DocketEventSSLContext *ssl_context = event->ctx;
    ssl_context->ssl_event_cb = cb;
    ssl_context->ssl_ctx = ctx;
    return;
  }
#endif
  event->event_cb = cb;
  event->ctx = ctx;
}

void devent_set_read_enable(DocketEvent *event, bool enable) {
  if (enable) {
    set_read_enable(event);
  } else {
    set_read_disable(event);
  }
}

void
DocketEvent_set_cb(DocketEvent *event, docket_event_read_callback read_cb, docket_event_write_callback write_cb,
                   docket_event_callback event_cb,
                   void *ctx) {
#ifdef DEVENT_SSL
  if (event->ssl) {
    DocketEventSSLContext *ssl_context = event->ctx;
    ssl_context->ssl_read_cb = read_cb;
    ssl_context->ssl_write_cb = write_cb;
    ssl_context->ssl_event_cb = event_cb;
    ssl_context->ssl_ctx = ctx;
    return;
  }
#endif

  event->read_cb = read_cb;
  event->write_cb = write_cb;
  event->event_cb = event_cb;
  event->ctx = ctx;
}

#ifdef DEVENT_SSL
static ssize_t DocketEvent_ssl_write(DocketEvent *event, const char *data, size_t len, bool force) {
  DocketEventSSLContext *ssl_context = event->ctx;
  DocketBuffer *out_buffer = ssl_context->ssl_out_buffer;

  int r = SSL_write(ssl_context->ssl, data, (int) len);
  if (r < 1) {
    int error = SSL_get_error(ssl_context->ssl, r);
    LOGI("BIO_write error: %d", ERR_error_string(error, NULL));
    devent_close_internal(event, DEVENT_WRITE | DEVENT_ERROR | DEVENT_OPENSSL);
    return -1;
  }

  int pending = BIO_pending(ssl_context->wbio);
  int bio_read_len = 0;

  Buffer *buffer = Docket_buffer_alloc();
  while (bio_read_len < pending && (r = BIO_read(ssl_context->wbio, buffer->data, sizeof(buffer->data))) > 0) {
    DocketBuffer_write(out_buffer, buffer->data, r);
    bio_read_len += r;
  }

  if (force) {
    SOCKET fd = event->fd;
    Docket *docket = event->docket;

    unsigned int ev = event->ev;
    set_write_enable(event);
    ssize_t ret;
    if (devent_write_data(event, out_buffer) == -1) {
      ret = -1;
    } else {
      ret = (ssize_t) len;
    }
    event = Docket_find_event(docket, fd);
    if (event) {
      event->ev = ev;
    }
    return ret;
  }
  if (devent_write_enable(event)) {
    return devent_write_data(event, out_buffer);
  }
  return -1;
}
#endif

ssize_t DocketEvent_write(DocketEvent *event, const char *data, size_t len) {
#ifdef DEVENT_SSL
  if (event->ssl) {
    return DocketEvent_ssl_write(event, data, len, false);
  }
#endif

  DocketBuffer *out_buffer = event->out_buffer;
  DocketBuffer_write(out_buffer, data, len);

  if (devent_write_enable(event)) {
    return devent_write_data(event, out_buffer);
  }
  return -1;
}

static ssize_t DocketEvent_ssl_write_buffer(DocketEvent *event, DocketBuffer *buffer, bool force) {
  Buffer *buf = Docket_buffer_alloc();
  ssize_t ret = 0;

  do {
    ssize_t r = DocketBuffer_read(buffer, buf->data, buf->len);
    if (r == -1) {
      return -1;
    }
    ret += (ssize_t) buf->len;
    r = DocketEvent_ssl_write(event, buf->data, r, force);

    if (r == -1) {
      return -1;
    }
  } while (DocketBuffer_length(buffer) > 0);
  return ret;
}

ssize_t DocketEvent_write_buffer(DocketEvent *event, DocketBuffer *buffer) {
#ifdef DEVENT_SSL
  if (event->ssl) {
    return DocketEvent_ssl_write_buffer(event, buffer, false);
  }
#endif

  DocketBuffer *out_buffer = event->out_buffer;
  if (out_buffer == NULL) {
    return -1;
  }
  DocketBuffer_moveto(out_buffer, buffer);
  if (devent_write_enable(event)) {
    return devent_write_data(event, out_buffer);
  }
  return -1;
}

ssize_t DocketEvent_write_buffer_force(DocketEvent *event, DocketBuffer *buffer) {
#ifdef DEVENT_SSL
  if (event->ssl) {
    return DocketEvent_ssl_write_buffer(event, buffer, true);
  }
#endif

  DocketBuffer *out_buffer = event->out_buffer;
  if (out_buffer == NULL) {
    return -1;
  }
  DocketBuffer_moveto(out_buffer, buffer);

  SOCKET fd = event->fd;
  Docket *docket = event->docket;

  unsigned int ev = event->ev;
  set_write_enable(event);
  ssize_t ret = devent_write_data(event, out_buffer);
  event = Docket_find_event(docket, fd);
  if (event) {
    event->ev = ev;
  }
  return ret;
}

ssize_t DocketEvent_read(DocketEvent *event, char *data, size_t len) {
  DocketBuffer *in_buffer = event->in_buffer;
#ifdef DEVENT_SSL
  if (event->ssl) {
    DocketEventSSLContext *ssl_context = event->ctx;
    in_buffer = ssl_context->ssl_in_buffer;
  }
#endif

  return DocketBuffer_read(in_buffer, data, len);
}

ssize_t DocketEvent_read_full(DocketEvent *event, char *data, size_t len) {
  DocketBuffer *in_buffer = event->in_buffer;
#ifdef DEVENT_SSL
  if (event->ssl) {
    DocketEventSSLContext *ssl_context = event->ctx;
    in_buffer = ssl_context->ssl_in_buffer;
  }
#endif
  return DocketBuffer_read_full(in_buffer, data, len);
}

ssize_t DocketEvent_peek(DocketEvent *event, char *data, size_t len) {
  DocketBuffer *in_buffer = event->in_buffer;
#ifdef DEVENT_SSL
  if (event->ssl) {
    DocketEventSSLContext *ssl_context = event->ctx;
    in_buffer = ssl_context->ssl_in_buffer;
  }
#endif
  return DocketBuffer_peek(in_buffer, data, len);
}

ssize_t DocketEvent_peek_full(DocketEvent *event, char *data, size_t len) {
  DocketBuffer *in_buffer = event->in_buffer;
#ifdef DEVENT_SSL
  if (event->ssl) {
    DocketEventSSLContext *ssl_context = event->ctx;
    in_buffer = ssl_context->ssl_in_buffer;
  }
#endif
  return DocketBuffer_peek_full(in_buffer, data, len);
}

bool DocketEvent_fetch_local_address(DocketEvent *event, struct sockaddr *address, socklen_t *socklen,
                                     unsigned short *port) {
  if (event->fd <= 0) {
    return false;
  }

  if (getsockname(event->fd, address, socklen) == -1) {
    LOGD("getsockname failed: %s", devent_errno());
    return false;
  }

  if (port != NULL) {
    if (address->sa_family == AF_INET) {
      *port = ntohs(((struct sockaddr_in *) address)->sin_port);
    } else if (address->sa_family == AF_INET6) {
      *port = ntohs(((struct sockaddr_in6 *) address)->sin6_port);
    } else {
      return false;
    }
  }

  return true;
}

void DocketEvent_free(DocketEvent *event) {
  if (event == NULL) {
    return;
  }
  LOGD("fd = %d", event->fd);

  if (event->fd > 0) {
    // remove all event
    event->ev = DEVENT_NONE;
    devent_update_events(event->docket->fd, event->fd, event->ev, DEVENT_MOD_DEL);
    Docket_remove_event(event);

#ifdef WIN32
    closesocket(event->fd);
    event->fd = INVALID_SOCKET;
#else
    close(event->fd);
    event->fd = 0;
#endif
  }
  event->docket = NULL;

  DocketBuffer_free(event->in_buffer);
  DocketBuffer_free(event->out_buffer);

  event->in_buffer = NULL;
  event->out_buffer = NULL;

  event->ctx = NULL;
  free(event->remote_address);
  event->remote_address = NULL;

#ifdef DEVENT_SSL
  if (event->ssl) {
    DocketEventSSLContext_free(event->ctx);
  }
#endif

  free(event);
}