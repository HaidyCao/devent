//
// Created by Haidy on 2021/11/20.
//
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#else
#include <resolv.h>
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
  if (dns_context == NULL)
    return;

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
    LOGE("read dns data failed: recvfrom %s failed: %s",
         sockaddr_to_string(dns_address, NULL, 0),
         devent_errno());
    on_dns_failed(dns_event, dns_context);
    return;
  }

  struct hostent *host = NULL;
  if (c_dns_parse_a(dns_buffer, (unsigned int) size, &host) != 0) {
    LOGE("parse %s failed: %s", sockaddr_to_string(dns_address, NULL, 0), devent_errno());
    on_dns_failed(dns_event, dns_context);
    return;
  }

  struct sockaddr_storage addr;
  socklen_t addr_len = sizeof(addr);
  if (c_dns_parse_first_ip(host, (struct sockaddr *) &addr, &addr_len, dns_context->port) == -1) {
    c_dns_free_hostent(host);
    LOGE("c_dns_parse_first_ip %s failed: %s",
         sockaddr_to_string(dns_address, NULL, 0),
         devent_errno());
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

bool Docket_get_dns_server(Docket *docket, struct sockaddr **address, socklen_t *address_len) {
  SOCK_ADDRESS dns_address = docket->dns_server.address;
  socklen_t socklen = docket->dns_server.socklen;
  // init dns name server
  if (dns_address == NULL) {
    if (docket->dns_server_callback != NULL && docket->dns_server_callback(address, address_len)) {
      return true;
    }

#ifdef WIN32
    ULONG buf_len = sizeof(FIXED_INFO);
    FIXED_INFO *fixed_info = malloc(sizeof(FIXED_INFO));

    DWORD res = GetNetworkParams(fixed_info, &buf_len);
    if (res == ERROR_BUFFER_OVERFLOW) {
      fixed_info = malloc(buf_len);
      if (fixed_info == NULL) {
        LOGE("malloc fixed_info failed");
        return -1;
      }
      res = GetNetworkParams(fixed_info, &buf_len);
    }

    if (res == NO_ERROR) {
      socklen = sizeof(struct sockaddr_in);
      dns_address = malloc(sizeof(struct sockaddr_in));
      dns_address->sa_family = AF_INET;
      ((struct sockaddr_in *) dns_address)->sin_port = htons(53);
      if (fixed_info->CurrentDnsServer) {
        if (inet_pton(AF_INET,
                      fixed_info->CurrentDnsServer->IpAddress.String,
                      &((struct sockaddr_in *) dns_address)->sin_addr) == 1) {
          docket->dns_server.address = dns_address;
          docket->dns_server.socklen = socklen;
        } else {
          // TODO default dns server
        }
      } else {
        if (inet_pton(AF_INET,
                      fixed_info->DnsServerList.IpAddress.String,
                      &((struct sockaddr_in *) dns_address)->sin_addr) == 1) {
          docket->dns_server.address = dns_address;
          docket->dns_server.socklen = socklen;
        } else {
          // TODO default dns server
        }
      }

      LOGD("dns_address: %s", sockaddr_to_string(dns_address, NULL, 0));
    }
#else
#ifdef __ANDROID__
    return false;
#else
    struct __res_state state;
    res_ninit(&state);

    if (state.nscount > 0) {
      for (int i = 0; i < state.nscount; ++i) {
        if (state.nsaddr_list[i].sin_family == AF_INET) {
          size_t len = sizeof(state.nsaddr_list[i]);
          dns_address = malloc(len);
          memcpy(dns_address, &state.nsaddr_list[i], len);
          docket->dns_server.address = dns_address;
          socklen = docket->dns_server.socklen = len;
          break;
        }
      }
    }

    res_nclose(&state);
    if (dns_address == NULL) {
      LOGE("dns server is null");
      return false;
    }
#endif
#endif
  }
  *address = dns_address;
  *address_len = socklen;
  return true;
}