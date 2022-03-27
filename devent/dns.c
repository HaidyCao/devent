//
// Created by Haidy on 2021/11/20.
//
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <sys/socket.h>
#endif

#include "dns.h"
#include "c_dns.h"
#include "event.h"
#include "event_def.h"
#include "docket_def.h"
#include "clib.h"
#include "log.h"
#include "utils_internal.h"
#include "connect.h"
#include "docket.h"

#define DNS_BUFFER_SIZE 4096

DocketDnsContext *DocketDnsContext_new(DocketEvent *event, char *domain, unsigned short port) {
  DocketDnsContext *ctx = calloc(1, sizeof(DocketDnsContext));
  ctx->event = event;
  ctx->domain = domain;
  ctx->port = port;

  return ctx;
}

void DocketDnsContext_free(DocketDnsContext *dns_context) {
  if (dns_context == NULL) return;

  free(dns_context->domain);
  free(dns_context);
}

static void on_dns_failed(DocketEvent *dns_event, DocketDnsContext *dns_context) {
  Docket *docket = dns_event->docket;
  DocketEvent *ev = dns_context->event;
  dns_context->event = NULL;

  // free dns event first
  DocketEvent_free(dns_event);
  DocketDnsContext_free(dns_context);

  SOCKET ev_fd = ev->fd;

  if (ev->event_cb) {
    ev->event_cb(ev, DEVENT_CONNECT | DEVENT_ERROR, ev->ctx);
  }
  ev = Docket_find_event(docket, ev_fd);
  if (ev) {
    DocketEvent_free(ev);
  }
}

void docket_dns_write(DocketEvent *event) {
  DocketDnsContext *dns_context = event->ctx;

  char dns_buffer[DNS_BUFFER_SIZE];
  ssize_t packet_len = c_dns_pack(dns_context->domain, dns_buffer, DNS_BUFFER_SIZE, C_DNS_QTYPE_A);
  if (packet_len == -1) {
    LOGE("dns pack failed");
    on_dns_failed(event, dns_context);
    return;
  }

  DocketEvent_write(event, dns_buffer, packet_len);
}

void docket_dns_read(DocketEvent *dns_event) {
  char dns_buffer[DNS_BUFFER_SIZE];
  DocketDnsContext *dns_context = dns_event->ctx;

  Docket *docket = dns_event->docket;
  SOCK_ADDRESS dns_address = docket->dns_server.address;
  if (dns_address == NULL) {
    LOGE("dns server is null");
    on_dns_failed(dns_event, dns_context);
    return;
  }

  ssize_t size = DocketEvent_read(dns_event, dns_buffer, DNS_BUFFER_SIZE);
  if (size < 0) {
    LOGE("read dns data failed: recvfrom %s failed: %s", sockaddr_to_string(dns_address, NULL, 0), devent_errno());
    on_dns_failed(dns_event, dns_context);
    return;
  }

  struct hostent *host = NULL;
  if (c_dns_parse_a(dns_buffer, size, &host) != 0) {
    LOGE("parse %s failed: %s", sockaddr_to_string(dns_address, NULL, 0), devent_errno());
    on_dns_failed(dns_event, dns_context);
    return;
  }

  struct sockaddr_storage addr;
  socklen_t addr_len = sizeof(addr);
  if (c_dns_parse_first_ip(host, (struct sockaddr *) &addr, &addr_len, dns_context->port) == -1) {
    c_dns_free_hostent(host);
    LOGE("c_dns_parse_first_ip %s failed: %s", sockaddr_to_string(dns_address, NULL, 0), devent_errno());
    on_dns_failed(dns_event, dns_context);
    return;
  }
  c_dns_free_hostent(host);

  LOGD("connect to %s", sockaddr_to_string((struct sockaddr *) &addr, NULL, 0));

  // connect remote
#ifdef DEVENT_SSL
  if (dns_context->event_ssl) {
    DocketEvent_connect_ssl(docket, dns_context->event->fd, (struct sockaddr *) &addr, addr_len);
  } else
#endif
  {
    DocketEvent_connect(docket, dns_context->event->fd, (struct sockaddr *) &addr, addr_len);
  }

  DocketEvent_free(dns_event);
  DocketDnsContext_free(dns_context);
}
