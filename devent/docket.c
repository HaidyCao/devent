//
// Created by Haidy on 2021/9/21.
//

#include <stdlib.h>
#include <strings.h>

#ifdef __APPLE__

#include <sys/event.h>

#elif __linux__ || __ANDROID__

#include <sys/epoll.h>

#endif

#include "docket.h"
#include "docket_def.h"
#include "listener_def.h"
#include "dns.h"
#include "log.h"
#include "event.h"
#include "event_def.h"
#include "utils_internal.h"
#include "clib.h"

Docket *Docket_new() {
  Docket *docket = calloc(1, sizeof(Docket));
#ifdef __APPLE__
  docket->fd = kqueue();
#elif __linux__ || __ANDROID__
  docket->fd = epoll_create1(0);
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
  SOCK_ADDRESS *address = malloc(sizeof(struct sockaddr_storage));
  socklen_t socklen = sizeof(struct sockaddr_storage);

  char s[strlen(server) + 4];
  if (strstr(server, ":")) {
    strcpy(s, server);
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

DocketListener *Docket_get_listener(Docket *docket, int listener_fd) {
  if (docket == NULL) {
    LOGE("docket is NULL");
    return NULL;
  }

  return CSparseArray_get(docket->listeners, listener_fd);
}

void Docket_add_event(DocketEvent *event) {
  Docket *docket = event->docket;
  CSparseArray_put(docket->events, event->fd, event);
}

void Docket_add_dns_event(DocketDnsEvent *event) {
  Docket *docket = event->docket;
  CSparseArray_put(docket->dns_events, event->dns_fd, event);
}

void Docket_remove_dns_event(DocketDnsEvent *event) {
  Docket *docket = event->docket;
  CSparseArray_remove(docket->dns_events, event->dns_fd);
}

void Docket_remove_event(DocketEvent *event) {
  Docket *docket = event->docket;
  CSparseArray_remove(docket->events, event->fd);
}

DocketEvent *Docket_find_event(Docket *docket, int fd) {
  if (docket == NULL)
    return NULL;
  return CSparseArray_get(docket->events, fd);
}

int Docket_get_fd(Docket *docket) {
  return docket->fd;
}
