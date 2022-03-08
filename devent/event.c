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

#include "event.h"
#include "buffer.h"
#include "write.h"
#include "utils_internal.h"
#include "docket.h"
#include "event_def.h"
#include "docket_def.h"
#include "log.h"
#include "dns.h"

DocketEvent *DocketEvent_new(Docket *docket, SOCKET fd, void *ctx) {
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
  event->ssl = NULL;
  event->ssl_handshaking = false;
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
    return event->in_buffer;
  }
  return NULL;
}

DocketBuffer *DocketEvent_get_out_buffer(DocketEvent *event) {
  if (event) {
    return event->out_buffer;
  }
  return NULL;
}

void DocketEvent_set_read_cb(DocketEvent *event, docket_event_read_callback cb, void *ctx) {
  event->read_cb = cb;
  event->ctx = ctx;
}

void DocketEvent_set_write_cb(DocketEvent *event, docket_event_write_callback cb, void *ctx) {
  event->write_cb = cb;
  event->ctx = ctx;
}

void DocketEvent_set_event_cb(DocketEvent *event, docket_event_callback cb, void *ctx) {
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
  event->read_cb = read_cb;
  event->write_cb = write_cb;
  event->event_cb = event_cb;
  event->ctx = ctx;
}

void DocketEvent_write(DocketEvent *event, const char *data, size_t len) {
  DocketBuffer *out_buffer = event->out_buffer;
  DocketBuffer_write(out_buffer, data, len);

  if (devent_write_enable(event)) {
    devent_write_data(event, out_buffer);
  }
}

void DocketEvent_write_buffer(DocketEvent *event, DocketBuffer *buffer) {
  DocketBuffer *out_buffer = event->out_buffer;
  if (out_buffer == NULL) {
    return;
  }
  DocketBuffer_moveto(out_buffer, buffer);
  devent_write_data(event, out_buffer);
}

void DocketEvent_write_buffer_force(DocketEvent *event, DocketBuffer *buffer) {
  DocketBuffer *out_buffer = event->out_buffer;
  if (out_buffer == NULL) {
    return;
  }
  DocketBuffer_moveto(out_buffer, buffer);

  int ev = event->ev;
  set_write_enable(event);
  devent_write_data(event, out_buffer);
  event->ev = ev;
}

ssize_t DocketEvent_read(DocketEvent *event, char *data, size_t len) {
  DocketBuffer *in_buffer = event->in_buffer;
  return DocketBuffer_read(in_buffer, data, len);
}

ssize_t DocketEvent_read_full(DocketEvent *event, char *data, size_t len) {
  return DocketBuffer_read_full(event->in_buffer, data, len);
}

ssize_t DocketEvent_peek(DocketEvent *event, char *data, size_t len) {
  DocketBuffer *in_buffer = event->in_buffer;
  return DocketBuffer_peek(in_buffer, data, len);
}

ssize_t DocketEvent_peek_full(DocketEvent *event, char *data, size_t len) {
  return DocketBuffer_peek_full(event->in_buffer, data, len);
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

  if (event->dns_event) {
    DocketDnsEvent_free(event->dns_event);
    event->dns_event = NULL;
  }

  if (event->fd > 0) {
    // remove all event
    event->ev = DEVENT_NONE;
    devent_update_events(event->docket->fd, event->fd, event->ev, 1);
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
    SSL_free(event->ssl);
    event->ssl = NULL;
  }
#endif

  free(event);
}