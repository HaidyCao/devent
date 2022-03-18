//
// Created by Haidy on 2021/11/20.
//
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#else
#include <sys/socket.h>
#include <unistd.h>

#include <netinet/in.h>
#include <resolv.h>
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
#include "win_def.h"

#define DNS_BUFFER_SIZE 4096

#ifdef WIN32
#include <iphlpapi.h>

#pragma comment(lib, "IPHLPAPI.lib")
#endif

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

DocketDnsEvent *DocketDnsEvent_new(Docket *docket, SOCKET fd, const char *domain, unsigned short port) {
  DocketDnsEvent *event = calloc(1, sizeof(DocketDnsEvent));
  event->docket = docket;
  event->dns_fd = fd;
  event->domain = strdup(domain);
  event->port = port;
  return event;
}

void DocketDnsEvent_free(DocketDnsEvent *event) {
  if (event == NULL) {
    return;
  }
  if (event->dns_fd != 0) {
    Docket_remove_dns_event(event);

    devent_update_events(event->docket->fd, event->dns_fd, DEVENT_NONE, 1);
#ifdef WIN32
    closesocket(event->dns_fd);
#else
    close(event->dns_fd);
#endif
  }

  if (event->event) {
    event->event->dns_event = NULL;
  }

  free(event->domain);
  free(event);
}

void docket_dns_write(DocketEvent *event) {
  DocketDnsContext *dns_context = event->ctx;

  char dns_buffer[DNS_BUFFER_SIZE];
  ssize_t packet_len = c_dns_pack(dns_context->domain, dns_buffer, DNS_BUFFER_SIZE, C_DNS_QTYPE_A);
  if (packet_len == -1) {
    LOGE("dns pack failed");
    DocketEvent *ev = dns_context->event;
    dns_context->event = NULL;

    // free dns event first
    ev->dns_event = NULL;
    DocketEvent_free(event);

    if (ev->event_cb) {
      ev->event_cb(ev, DEVENT_CONNECT | DEVENT_ERROR, ev->ctx);
    }
    return;
  }

  DocketEvent_write(event, dns_buffer, packet_len);
}

void devent_dns_write(DocketDnsEvent *event) {
  LOGD("DNS: %s", event->domain);
  char dns_buffer[DNS_BUFFER_SIZE];
  ssize_t packet_len = c_dns_pack(event->domain, dns_buffer, DNS_BUFFER_SIZE, C_DNS_QTYPE_A);
  if (packet_len == -1) {
    LOGE("dns pack failed");
    DocketEvent *ev = event->event;
    event->event = NULL;

    // free dns event first
    ev->dns_event = NULL;
    DocketDnsEvent_free(event);

    if (ev->event_cb) {
      ev->event_cb(ev, DEVENT_CONNECT | DEVENT_ERROR, ev->ctx);
    }
    return;
  }

  Docket *docket = event->docket;
  SOCK_ADDRESS dns_address = docket->dns_server.address;
  socklen_t socklen = docket->dns_server.socklen;
  // init dns name server
  if (dns_address == NULL) {
#ifdef WIN32
    ULONG buf_len = sizeof(FIXED_INFO);
    FIXED_INFO *fixed_info = malloc(sizeof(FIXED_INFO));

    DWORD res = GetNetworkParams(fixed_info, &buf_len);
    if (res == ERROR_BUFFER_OVERFLOW) {
      fixed_info = malloc(buf_len);
      if (fixed_info == NULL) {
        LOGE("malloc fixed_info failed");
        return;
      }
      res = GetNetworkParams(fixed_info, &buf_len);
    }

    if (res == NO_ERROR) {
      socklen = sizeof(struct sockaddr_in);
      dns_address = malloc(sizeof(struct sockaddr_in));
      if (fixed_info->CurrentDnsServer) {
        inet_pton(AF_INET, fixed_info->CurrentDnsServer->IpAddress.String, dns_address);
        docket->dns_server.address = dns_address;
        docket->dns_server.socklen = socklen;
      } else {
        inet_pton(AF_INET, fixed_info->DnsServerList.IpAddress.String, dns_address);
        docket->dns_server.address = dns_address;
        docket->dns_server.socklen = socklen;
      }
    }
#else
    struct __res_state state;
    res_ninit(&state);

    if (state.nscount > 0) {
      size_t len = sizeof(state.nsaddr_list[0]);
      dns_address = malloc(len);
      memcpy(dns_address, &state.nsaddr_list[0], len);
      docket->dns_server.address = dns_address;
      docket->dns_server.socklen = len;
    }

    res_nclose(&state);
        if (dns_address == NULL) {
      LOGE("dns server is null");
      return;
    }
#endif
  }

  ssize_t size = sendto(event->dns_fd, dns_buffer, packet_len, 0, dns_address, socklen);
  if (size == -1) {
    LOGE("sendto failed: %s", devent_errno());
    return;
  }

  // read only
  devent_update_events(docket->fd, event->dns_fd, DEVENT_READ, 1);
}

static void on_dns_failed(DocketEvent *dns_event, DocketDnsContext *dns_context) {
  Docket *docket = dns_event->docket;
  DocketEvent *ev = dns_context->event;
  dns_context->event = NULL;

  // free dns event first
  ev->dns_event = NULL;
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

void docket_dns_read(DocketEvent *dns_event) {
  char dns_buffer[DNS_BUFFER_SIZE];
  DocketDnsContext *dns_context = dns_event->ctx;

  Docket *docket = dns_event->docket;
  SOCK_ADDRESS dns_address = docket->dns_server.address;
  if (dns_address == NULL) {
    LOGE("dns server is null");
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

void devent_dns_read(DocketDnsEvent *event) {
  LOGD("");
  char dns_buffer[DNS_BUFFER_SIZE];

  Docket *docket = event->docket;
  SOCK_ADDRESS dns_address = docket->dns_server.address;
  socklen_t socklen = docket->dns_server.socklen;
  if (dns_address == NULL) {
    LOGE("dns server is null");
    return;
  }

  ssize_t size = recvfrom(event->dns_fd, dns_buffer, DNS_BUFFER_SIZE, 0, dns_address, &socklen);
  if (size < 0) {
    LOGE("recvfrom %s failed: %s", sockaddr_to_string(dns_address, NULL, 0), devent_errno());
    DocketEvent *ev = event->event;
    event->event = NULL;

    // free dns event first
    ev->dns_event = NULL;
    DocketDnsEvent_free(event);

    if (ev->event_cb) {
      ev->event_cb(ev, DEVENT_CONNECT | DEVENT_ERROR, ev->ctx);
    }
    return;
  }

  struct hostent *host = NULL;
  if (c_dns_parse_a(dns_buffer, size, &host) != 0) {
    LOGE("parse %s failed: %s", sockaddr_to_string(dns_address, NULL, 0), devent_errno());
    DocketEvent *ev = event->event;
    event->event = NULL;

    // free dns event first
    ev->dns_event = NULL;
    DocketDnsEvent_free(event);

    if (ev->event_cb) {
      ev->event_cb(ev, DEVENT_CONNECT | DEVENT_ERROR, ev->ctx);
    }
    return;
  }
  LOGD("dns event = %p", event);

  struct sockaddr_storage addr;
  socklen_t addr_len = sizeof(addr);
  if (c_dns_parse_first_ip(host, (struct sockaddr *) &addr, &addr_len, event->port) == -1) {
    c_dns_free_hostent(host);
    LOGE("c_dns_parse_first_ip %s failed: %s", sockaddr_to_string(dns_address, NULL, 0), devent_errno());
    DocketEvent *ev = event->event;
    event->event = NULL;

    // free dns event first
    ev->dns_event = NULL;
    DocketDnsEvent_free(event);

    if (ev->event_cb) {
      ev->event_cb(ev, DEVENT_CONNECT | DEVENT_ERROR, ev->ctx);
    }
    return;
  }
  c_dns_free_hostent(host);

  // add event to docket
  if (event->event->fd == -1) {
    event->event->fd = socket(addr.ss_family, SOCK_STREAM, IPPROTO_IP);
    Docket_add_event(event->event);
  }

  LOGD("connect to %s", sockaddr_to_string((struct sockaddr *) &addr, NULL, 0));

  // connect remote
#ifdef DEVENT_SSL
  if (event->event_ssl) {
    DocketEvent_connect_ssl(docket, event->event->fd, (struct sockaddr *) &addr, addr_len);
  } else
#endif
  {
    DocketEvent_connect(docket, event->event->fd, (struct sockaddr *) &addr, addr_len);
  }

  // free dns event
  DocketDnsEvent_free(event);
}