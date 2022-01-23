//
// Created by Haidy on 2021/11/13.
//
#include <stdbool.h>
#include <netinet/in.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <strings.h>

#include "clib.h"
#include "connect.h"
#include "utils_internal.h"
#include "log.h"
#include "docket.h"
#include "event_def.h"
#include "dns.h"
#include "docket_def.h"

static int new_fd(int fd_type, struct sockaddr *address) {
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
                             int fd,
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

  if (fd == -1) {
    fd = new_fd(fd_type, address);
  }

  if (fd == -1) {
    return NULL;
  }

  if (devent_turn_on_flags(fd, O_NONBLOCK)) {
    LOGD("turn_on_flags failed: %s", devent_errno());
    return NULL;
  }

  if (fd_type == SOCK_STREAM) {
    int cr = connect(fd, address, socklen);

    if (cr == -1 && errno != EINPROGRESS) {
      close(fd);
      return NULL;
    }
  } else {
    struct sockaddr_in local_addr;
    size_t local_addr_len = sizeof(local_addr);
    bzero(&local_addr, local_addr_len);
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(0);
    local_addr.sin_family = AF_INET;
    if (bind(fd, (struct sockaddr *) &local_addr, local_addr_len) == -1) {
      LOGD("bind udp failed: fd = %d", fd);
      close(fd);
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
  devent_update_events(Docket_get_fd(docket), dns_fd, DEVENT_READ_WRITE, 0);

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
  devent_update_events(Docket_get_fd(docket), fd, DEVENT_READ_WRITE, 0);

  Docket_add_event(event);
  return event;
}