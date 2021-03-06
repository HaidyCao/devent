//
// Created by Haidy on 2021/11/20.
//

#ifndef DOCKET_EVENT_DEF_H
#define DOCKET_EVENT_DEF_H

#include <stdbool.h>
#include <sys/socket.h>

#ifdef DEVENT_SSL
#include "openssl/ssl.h"
#include "openssl/err.h"

#endif

struct docket_event {
  Docket *docket;

  int fd;

  /**
   * for udp
   */
  struct sockaddr *remote_address;
  /**
   * for udp
   */
  socklen_t remote_address_len;

  /**
   * event callback
   */
  docket_event_callback event_cb;

  /**
   * read callback
   */
  docket_event_read_callback read_cb;

  /**
   * write callback
   */
  docket_event_write_callback write_cb;

  /**
   * for write
   */
  DocketBuffer *out_buffer;

  /**
   * for read
   */
  DocketBuffer *in_buffer;

  /**
   * ctx
   */
  void *ctx;

  /**
   * dns event
   */
  DocketDnsEvent *dns_event;

  /**
   * is connected
   */
  bool connected;

  /**
   * read write
   */
  unsigned char ev;

#ifdef DEVENT_SSL
  SSL *ssl;
  bool ssl_handshaking;

  DocketListener *listener;
#endif
};

#define set_write_disable(event) event->ev &= ~DEVENT_WRITE
#define set_write_enable(event) event->ev |= DEVENT_WRITE
#define set_read_disable(event) event->ev &= ~DEVENT_READ
#define set_read_enable(event) event->ev |= DEVENT_READ

#define devent_write_enable(event) (((event)->ev & DEVENT_WRITE) != 0)
#define devent_read_enable(event) (((event)->ev & DEVENT_READ) != 0)

#endif //DOCKET_EVENT_DEF_H
