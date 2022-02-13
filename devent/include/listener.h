//
// Created by Haidy on 2021/9/25.
//

#ifndef DOCKET_LISTENER_H
#define DOCKET_LISTENER_H

#ifndef _MSC_VER
#include <sys/socket.h>
#endif

#include "def.h"

#ifndef DOCKET_LISTENER_BACKLOG
#define DOCKET_LISTENER_BACKLOG 20
#endif

#ifdef DEVENT_SSL
#include "openssl/err.h"
#include "openssl/ssl.h"

#endif

typedef void (*docket_connect_cb)(DocketListener *l, int fd,
                                  struct sockaddr *address,
                                  socklen_t address_len, DocketEvent *event,
                                  void *ctx);

DocketListener *Docket_create_listener(Docket *docket, docket_connect_cb cb,
                                       int fd, struct sockaddr *address,
                                       socklen_t socklen, void *ctx);

#ifdef DEVENT_SSL

DocketListener *Docket_create_ssl_listener(Docket *docket, SSL_CTX *ssl_ctx,
                                           int (*cert_verify_callback)(X509_STORE_CTX *, void *),
                                           docket_connect_cb cb, int fd,
                                           struct sockaddr *address,
                                           socklen_t socklen, void *ctx);

DocketListener *Docket_create_ssl_listener_by_path(
    Docket *docket, const char *cert_path, const char *key_path,
    int (*cert_verify_callback)(X509_STORE_CTX *, void *),
    docket_connect_cb cb, int fd, struct sockaddr *address, socklen_t socklen,
    void *ctx);

#endif

#endif  // DOCKET_LISTENER_H
