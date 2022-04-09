//
// Created by Haidy on 2021/11/13.
//
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#ifdef WIN32
#include <WinSock2.h>
#include <MSWSock.h>

static GUID GuidConnectex = WSAID_CONNECTEX;
static LPFN_CONNECTEX lpfnConnectex = NULL;
#else
#include <netinet/in.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <strings.h>

#endif

#include "clib.h"
#include "connect.h"
#include "utils_internal.h"
#include "log.h"
#include "docket.h"
#include "event_def.h"
#include "dns.h"
#include "docket_def.h"
#include "win_def.h"
#include "event_ssl.h"

static void on_dns_read(DocketEvent *event, void *ctx) {
  docket_dns_read(event);
}

static SOCKET new_fd(int fd_type, struct sockaddr *address) {
  int sa_family;
  if (address != NULL) {
    if (address->sa_family != AF_INET && address->sa_family != AF_INET6) {
      return -1;
    }
    sa_family = address->sa_family;
  } else {
    sa_family = AF_INET;
  }

//  if (fd_type == SOCK_DGRAM) {
//    return socket(sa_family, fd_type, IPPROTO_UDP);
//  } else {
#ifdef WIN32
  return WSASocketW(sa_family, fd_type, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
#else
  return socket(sa_family, fd_type, 0);
#endif
//  }
}

static DocketEvent *
DocketEvent_connect_internal(Docket *docket,
                             SOCKET fd,
                             int fd_type,
                             struct sockaddr *address,
                             socklen_t socklen
) {
  if (docket == NULL) {
    return NULL;
  }

  if (fd == -1) {
    fd = new_fd(fd_type, address);
  }

  if (fd == -1) {
    return NULL;
  }

#ifndef WIN32
  if (devent_turn_on_flags(fd, O_NONBLOCK)) {
    LOGD("turn_on_flags failed: %s", devent_errno());
    return NULL;
  }
#endif

  if (fd_type == SOCK_STREAM) {
#ifdef WIN32
    if (lpfnConnectex == NULL) {
      DWORD dwBytes;
      int r = WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER,
                       &GuidConnectex, sizeof(GuidConnectex),
                       &lpfnConnectex, sizeof(lpfnConnectex),
                       &dwBytes, NULL, NULL);
      if (r == SOCKET_ERROR) {
        LOGE("WSAIoctl failed with error: %u\n", WSAGetLastError());
        closesocket(fd);
        return NULL;
      }
    }
    IO_CONTEXT *io = IO_CONTEXT_new(IOCP_OP_CONNECT, fd);
    io->remote = address;
    io->remote_len = socklen;

    // local
    io->local_len = sizeof(struct sockaddr_storage);
    io->local = calloc(1, sizeof(struct sockaddr_storage));
    io->local->sa_family = address->sa_family;
    if (io->local->sa_family == AF_INET) {
      bzero(&((struct sockaddr_in *) io->local)->sin_addr, sizeof(struct in_addr));
      ((struct sockaddr_in *) io->local)->sin_port = htons(0);
    } else if (io->local->sa_family == AF_INET6) {
      ((struct sockaddr_in6 *) io->local)->sin6_addr = in6addr_any;
      ((struct sockaddr_in6 *) io->local)->sin6_port = htons(0);
    }

    // before connect we must call bind function first
    if (bind(fd, io->local, io->local_len) == SOCKET_ERROR) {
      LOGE("bind failed with error: %u\n", WSAGetLastError());
      closesocket(fd);
      return NULL;
    }

    CreateIoCompletionPort((HANDLE) fd, docket->fd, fd, 0);

    if (!lpfnConnectex(fd, address, socklen, NULL, 0, NULL, (LPOVERLAPPED) io)) {
      if (WSAGetLastError() != WSA_IO_PENDING) {
        LOGE("connect failed with error: %u\n", WSAGetLastError());
        closesocket(fd);
        return NULL;
      }
    }
#else
    int cr = connect(fd, address, socklen);

    if (cr == -1 && errno != EINPROGRESS) {
      close(fd);
      return NULL;
    }
#endif
  } else {
    struct sockaddr_in local;
    socklen_t local_len = sizeof(local);
    bzero(&local, local_len);
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons(0);
    local.sin_family = AF_INET;
    if (bind(fd, (struct sockaddr *) &local, local_len) == -1) {
      LOGD("bind failed: fd = %d, %s", fd, devent_errno());
#ifdef WIN32
      closesocket(fd);
#else
      close(fd);
#endif
      return NULL;
    }

#ifdef WIN32
    CreateIoCompletionPort((HANDLE) fd, docket->fd, fd, 0);
#endif
  }

  DocketEvent *event = Docket_find_event(docket, fd);
  if (event == NULL) {
    event = DocketEvent_new(docket, fd, NULL);
    event->connected = false;

    // add event to docket
    Docket_add_event(event);
  }
  event->ev = DEVENT_READ_WRITE_ET;

  if (devent_update_events(docket->fd, fd, event->ev, DEVENT_MOD_ADD) == -1) {
    return NULL;
  }

  // udp
  if (fd_type == SOCK_DGRAM) {
    event->remote_address = malloc(socklen);
    memcpy(event->remote_address, address, socklen);
    event->remote_address_len = socklen;
    event->connected = true;

#ifdef WIN32
    // prepare for read data
    IO_CONTEXT *io = IO_CONTEXT_new(IOCP_OP_READ, fd);
    io->lpFlags = 0;

    if (WSARecvFrom(io->socket, &io->buf, 1,
                    NULL, &io->lpFlags, event->remote_address, &event->remote_address_len, (LPWSAOVERLAPPED) io, NULL)
        == SOCKET_ERROR
        && WSAGetLastError() != WSA_IO_PENDING) {
      LOGI("WSARecv failed: %d", WSAGetLastError());
      closesocket(io->socket);
      DocketEvent_free(event);
      IO_CONTEXT_free(io);
      return NULL;
    }
#endif
  }
  return event;
}

static
DocketEvent *DocketEvent_connect_hostname_internal(Docket *docket, SOCKET fd, const char *host, unsigned short port
#ifdef DEVENT_SSL
    , bool ssl
#endif
) {
  // check ipv4 or ipv6
  if (docket == NULL || host == NULL) {
    LOGI("docket or host is NULL");
    return NULL;
  }
  LOGD("connect to %s %d", host, port);

  bool is_domain = true;
  char *fmt = NULL;
  if (host[0] == '[') {
    is_domain = false;
    fmt = "[%s]:%d";
  } else {
    unsigned char result[4];
    if (devent_parse_ipv4(host, result)) {
      is_domain = false;
      fmt = "%s:%d";
    }
  }

  if (!is_domain) {
    struct sockaddr address;
    socklen_t socklen = sizeof(struct sockaddr);
    char address_[1024];

    sprintf(address_, fmt, host, port);

    if (parse_address(address_, &address, &socklen) != -1) {
      return DocketEvent_connect_internal(docket, fd, SOCK_STREAM, &address, socklen);
    }
  }

  if (fd == -1) {
    fd = new_fd(SOCK_STREAM, NULL);
  }

  DocketEvent *event = DocketEvent_new(docket, fd, NULL);
  event->connected = false;

  // create dns event
  struct sockaddr *dns_server_address;
  socklen_t dns_server_address_len;
  Docket_get_dns_server(docket, &dns_server_address, &dns_server_address_len);
  LOGD("dns server: %s, len = %d", sockaddr_to_string(dns_server_address, NULL, 0), dns_server_address_len);

  DocketEvent *dns = DocketEvent_create_udp(docket, -1, dns_server_address, dns_server_address_len);
  DocketDnsContext *ctx = DocketDnsContext_new(event, strdup(host), port);
#ifdef DEVENT_SSL
  ctx->event_ssl = ssl;
#endif

  // read dns packet
  DocketEvent_set_read_cb(dns, on_dns_read, ctx);
  docket_dns_write(dns);

  Docket_add_event(event);
  return event;
}

DocketEvent *DocketEvent_connect(Docket *docket, SOCKET fd, struct sockaddr *address, socklen_t socklen) {
  return DocketEvent_connect_internal(docket, fd, SOCK_STREAM, address, socklen);
}

DocketEvent *DocketEvent_create_udp(Docket *docket, SOCKET fd, struct sockaddr *address, socklen_t socklen) {
  return DocketEvent_connect_internal(docket, fd, SOCK_DGRAM, address, socklen);
}

#ifdef DEVENT_SSL
static void prepare_and_get_event_ssl(DocketEvent *event) {
  if (event == NULL || event->ssl) {
    return;
  }

  DocketEventSSLContext *event_ssl_context = DocketEventSSLContext_new(event->docket->ssl_ctx, false, 0);
  DocketEvent_set_cb(event, docket_on_ssl_read, docket_on_ssl_write, docket_on_ssl_event, event_ssl_context);
  event->ssl = true;

  SSL_set_connect_state(event_ssl_context->ssl);
}

DocketEvent *DocketEvent_connect_ssl(Docket *docket, SOCKET fd, struct sockaddr *address, socklen_t socklen) {
  DocketEvent *event = DocketEvent_connect_internal(docket, fd, SOCK_STREAM, address, socklen);
  prepare_and_get_event_ssl(event);
  return event;
}

DocketEvent *DocketEvent_connect_hostname_ssl(Docket *docket,
                                              int fd,
                                              const char *host,
                                              unsigned short port) {
  DocketEvent *event = DocketEvent_connect_hostname_internal(docket, fd, host, port, true);
  prepare_and_get_event_ssl(event);
  return event;
}

#endif

DocketEvent *DocketEvent_connect_hostname(Docket *docket, SOCKET fd, const char *host, unsigned short port) {
  return DocketEvent_connect_hostname_internal(docket, fd, host, port
#ifdef DEVENT_SSL
      , false
#endif
  );
}

DocketEvent *DocketEvent_create(Docket *docket, SOCKET fd) {
  DocketEvent *event = DocketEvent_new(docket, fd, NULL);
  event->connected = true;
  event->ev = DEVENT_READ_WRITE_ET;

#ifdef WIN32
#else
  devent_turn_on_flags(fd, O_NONBLOCK);
  devent_update_events(docket->fd, fd, event->ev, DEVENT_MOD_ADD);
#endif

  Docket_add_event(event);
  return event;
}