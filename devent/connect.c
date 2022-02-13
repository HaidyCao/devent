//
// Created by Haidy on 2021/11/13.
//
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#ifdef WIN32
#include <winsock.h>
#include <MSWSock.h>
#define bzero(p, s) memset(p, 0, s)

static GUID GuidConnectex = WSAID_CONNECTEX;
static LPFN_CONNECTEX lpfnConnectex = NULL;
#else
#include <netinet/in.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <strings.h>

typedef closesocket(fd) close(fd)
#endif

#include "clib.h"
#include "connect.h"
#include "utils_internal.h"
#include "log.h"
#include "docket.h"
#include "event_def.h"
#include "dns.h"
#include "docket_def.h"

static unsigned int new_fd(int fd_type, struct sockaddr *address) {
  if (address->sa_family != AF_INET && address->sa_family != AF_INET6) {
    return -1;
  }

  if (fd_type == SOCK_DGRAM) {
    return socket(address->sa_family, fd_type, IPPROTO_UDP);
  } else {
    return socket(address->sa_family, fd_type, 0);
  }
}

static DocketEvent *
DocketEvent_connect_internal(Docket *docket,
                             SOCKET fd,
                             int fd_type,
                             struct sockaddr *address,
                             socklen_t socklen
#ifdef DEVENT_SSL
    , bool ssl
#endif
) {
  if (docket == NULL) {
    return NULL;
  }

  if (fd == -1 || fd == 0) {
    fd = new_fd(fd_type, address);
  }

  if (fd == -1 || fd == 0) {
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
    IO_CONTEXT *io = calloc(1, sizeof(IO_CONTEXT));
    io->socket = fd;
    io->op = IOCP_OP_CONNECT;
    io->remote = address;
    io->remote_len = socklen;

    // local
    io->local_len = sizeof(struct sockaddr_storage);
    io->local = malloc(io->local_len);

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
    // TODO test
    struct sockaddr_in local_addr;
    size_t local_addr_len = sizeof(local_addr);
    bzero(&local_addr, local_addr_len);
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(0);
    local_addr.sin_family = AF_INET;
    if (bind(fd, (struct sockaddr *) &local_addr, local_addr_len) == -1) {
      LOGD("bind udp failed: fd = %d", fd);
      closesocket(fd);
      return NULL;
    }
  }

  DocketEvent *event = Docket_find_event(docket, fd);
  if (event == NULL) {
    event = DocketEvent_new(docket, fd, NULL);
    event->connected = false;

    // add event to docket
    Docket_add_event(event);
  }
  event->ev = DEVENT_READ_WRITE;

  if (devent_update_events(docket->fd, fd, event->ev, 0) == -1) {
    return NULL;
  }

#ifdef DEVENT_SSL
  if (ssl) {
    event->ssl = SSL_new(docket->ssl_ctx);
    event->ssl_handshaking = true;
    if (!SSL_set_fd(event->ssl, event->fd)) {
      LOGE("SSL_set_fd failed: %s", ERR_error_string(ERR_get_error(), NULL));
    } else {
      SSL_set_connect_state(event->ssl);
    }
  }
#endif

  // udp
  if (fd_type == SOCK_DGRAM) {
    event->remote_address = malloc(socklen);
    memcpy(event->remote_address, address, socklen);
    event->remote_address_len = socklen;
    event->connected = true;
    // TODO udp buffer
  }
  return event;
}

static
DocketEvent *DocketEvent_connect_hostname_internal(Docket *docket, int fd, const char *host, unsigned short port
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
      return DocketEvent_connect(docket, fd, &address, socklen);
    }
  }

  DocketEvent *event = DocketEvent_new(docket, fd, NULL);
  event->connected = false;

  int dns_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (dns_fd == -1) {
    LOGE("create dns fd failed: %s", devent_errno());
    return NULL;
  }
  devent_turn_on_flags(dns_fd, O_NONBLOCK);
  devent_update_events(docket->fd, dns_fd, DEVENT_READ_WRITE, 0);

  DocketDnsEvent *dns_event = DocketDnsEvent_new(docket, dns_fd, host, port);
  dns_event->event = event;
  event->dns_event = dns_event;

#ifdef DEVENT_SSL
  dns_event->event_ssl = ssl;
#endif

  Docket_add_dns_event(dns_event);
  return event;
}

DocketEvent *DocketEvent_connect(Docket *docket, int fd, struct sockaddr *address, socklen_t socklen) {
  return DocketEvent_connect_internal(docket, fd, SOCK_STREAM, address, socklen
#ifdef DEVENT_SSL
      , false
#endif
  );
}

#ifdef DEVENT_SSL
DocketEvent *DocketEvent_connect_ssl(Docket *docket, int fd, struct sockaddr *address, socklen_t socklen) {
  return DocketEvent_connect_internal(docket, fd, SOCK_STREAM, address, socklen, true);
}

DocketEvent *DocketEvent_connect_hostname_ssl(Docket *docket,
                                              int fd,
                                              const char *host,
                                              unsigned short port) {
  return DocketEvent_connect_hostname_internal(docket, fd, host, port, true);
}

#endif

DocketEvent *DocketEvent_connect_hostname(Docket *docket, int fd, const char *host, unsigned short port) {
  return DocketEvent_connect_hostname_internal(docket, fd, host, port
#ifdef DEVENT_SSL
      , false
#endif
  );
}

DocketEvent *DocketEvent_create(Docket *docket, int fd) {
  DocketEvent *event = DocketEvent_new(docket, fd, NULL);
  event->connected = true;

  devent_turn_on_flags(fd, O_NONBLOCK);
  devent_update_events(docket->fd, fd, DEVENT_READ_WRITE, 0);

  Docket_add_event(event);
  return event;
}