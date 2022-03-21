//
// Created by Haidy on 2021/11/20.
//

#ifndef DOCKET_EVENT_DEF_H
#define DOCKET_EVENT_DEF_H

#include <stdbool.h>
#include "docket_def.h"
#include "event.h"

#ifdef DEVENT_SSL
#include "openssl/ssl.h"
#include "openssl/err.h"
#endif

struct docket_event {
  Docket *docket;

  SOCKET fd;

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

#ifdef WIN32
  bool is_file;
#endif

  /**
   * read write
   */
  unsigned char ev;

#ifdef DEVENT_SSL
  /**
   * is ssl event
   */
  DocketEventSSL *ssl;
#endif
};

#define set_write_disable(event) event->ev &= ~DEVENT_WRITE
#define set_write_enable(event) event->ev |= DEVENT_WRITE
#define set_read_disable(event) event->ev &= ~DEVENT_READ
#define set_read_enable(event) event->ev |= DEVENT_READ

#define devent_write_enable(event) (((event)->ev & DEVENT_WRITE) != 0)
#define devent_read_enable(event) (((event)->ev & DEVENT_READ) != 0)

#endif //DOCKET_EVENT_DEF_H
