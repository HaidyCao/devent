//
// Created by Haidy on 2021/9/21.
//

#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__

#include <sys/event.h>

#elif __linux__ || __ANDROID__

#include <sys/epoll.h>

#elif __CYGWIN__

#include <ioapiset.h>
#include <handleapi.h>

#endif

#ifdef WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#else
#include <resolv.h>
#endif

#include "docket.h"
#include "docket_def.h"
#include "listener_def.h"
#include "dns.h"
#include "log.h"
#include "event.h"
#include "event_def.h"
#include "clib.h"
#include "utils.h"

#ifdef WIN32

#ifdef WIN32
#include <iphlpapi.h>

#pragma comment(lib, "IPHLPAPI.lib")
#endif

IO_CONTEXT *IO_CONTEXT_new(int op, SOCKET socket) {
  IO_CONTEXT *ctx = calloc(1, sizeof(IO_CONTEXT));
  ctx->op = op;
  ctx->socket = socket;
  if (ctx->op == IOCP_OP_READ) {
    ctx->buf.len = DOCKET_RECV_BUF_SIZE;
    ctx->buf.buf = malloc(DOCKET_RECV_BUF_SIZE);
  } else {
    ctx->buf.buf = NULL;
  }
  return ctx;
}

void IO_CONTEXT_free(IO_CONTEXT *ctx) {
  if (ctx == NULL) return;

  free(ctx->buf.buf);
  free(ctx->lpOutputBuffer);

  free(ctx);
}

#endif

Docket *Docket_new() {
#ifdef WIN32
  WSADATA lpWsaData;
  int startUpResult = WSAStartup(WINSOCK_VERSION, (LPWSADATA) &lpWsaData);
  if (startUpResult != 0) {
    LOGD("WSAStartup failed: %s", devent_errno());
    return NULL;
  }
#endif

  Docket *docket = calloc(1, sizeof(Docket));
#ifdef __APPLE__
  docket->fd = kqueue();
#elif __linux__ || __ANDROID__
  docket->fd = epoll_create1(0);

#elif WIN32
  docket->fd = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
#else
  free(docket);
  docket = NULL;
#endif

//    docket->dns_servers = CLinkedList_new();
  docket->dns_server.address = NULL;
  docket->dns_server.socklen = 0;
  docket->listeners = CSparseArray_new();
  docket->events = CSparseArray_new();
  docket->dns_events = CSparseArray_new();

#ifdef DEVENT_SSL
  SSL_load_error_strings();
  SSL_library_init();

  docket->ssl_ctx = SSL_CTX_new(SSLv23_client_method());
#endif

  return docket;
}

int Docket_set_dns_server(Docket *docket, char *server) {
  SOCK_ADDRESS address = malloc(sizeof(struct sockaddr_storage));
  socklen_t socklen = sizeof(struct sockaddr_storage);

  char s[1024];
  if (strstr(server, ":")) {
    sprintf(s, "%s", server);
  } else {
    sprintf(s, "%s:53", server);
  }

  if (parse_address(s, address, &socklen) == -1) {
    LOGE("parse_address");
    free(address);
    return -1;
  }

  if (docket->dns_server.address) {
    free(docket->dns_server.address);
    docket->dns_server.address = NULL;
  }

  docket->dns_server.address = address;
  docket->dns_server.socklen = socklen;
  return 0;
}

void Docket_add_listener(Docket *docket, DocketListener *listener) {
  if (docket == NULL) {
    LOGE("docket is NULL");
    return;
  }

  CSparseArray_put(docket->listeners, listener->fd, listener);
}

DocketListener *Docket_get_listener(Docket *docket, SOCKET fd) {
  if (docket == NULL) {
    LOGE("docket is NULL");
    return NULL;
  }

  return CSparseArray_get(docket->listeners, fd);
}

void Docket_add_event(DocketEvent *event) {
  Docket *docket = event->docket;
  CSparseArray_put(docket->events, event->fd, event);
}

void Docket_remove_event(DocketEvent *event) {
  Docket *docket = event->docket;
  CSparseArray_remove(docket->events, event->fd);
}

DocketEvent *Docket_find_event(Docket *docket, SOCKET fd) {
  if (docket == NULL)
    return NULL;
  return CSparseArray_get(docket->events, fd);
}

int Docket_get_dns_server(Docket *docket, struct sockaddr **address, socklen_t *address_len) {
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
    struct __res_state state;
    res_ninit(&state);

    if (state.nscount > 0) {
      size_t len = sizeof(state.nsaddr_list[0]);
      dns_address = malloc(len);
      memcpy(dns_address, &state.nsaddr_list[0], len);
      docket->dns_server.address = dns_address;
      socklen = docket->dns_server.socklen = len;
    }

    res_nclose(&state);
    if (dns_address == NULL) {
      LOGE("dns server is null");
      return -1;
    }
#endif
  }
  *address = dns_address;
  *address_len = socklen;

  return 0;
}