//
// Created by Haidy on 2021/9/25.
//

#include <stddef.h>
#ifndef WIN32
#include <sys/fcntl.h>
#else
#endif

#include "listener_def.h"
#include "utils_internal.h"
#include "docket_def.h"
#include "docket.h"
#include "log.h"
#include "event.h"

#ifdef DEVENT_SSL
#include "openssl/err.h"
#endif

static DocketListener *
Docket_create_listener_internal(Docket *docket,
#ifdef DEVENT_SSL
    SSL_CTX *ssl_ctx,
    int (*cert_verify_callback)(X509_STORE_CTX *, void *),
#endif
                                docket_connect_cb cb,
                                SOCKET fd,
                                struct sockaddr *address, socklen_t socklen,
                                void *ctx) {
  if (fd == -1) {
#ifndef WIN32
    fd = socket(address->sa_family, SOCK_STREAM, 0);
    if (fd == -1) {
      LOGD("docket_create_listener failed: fd = -1, reason = %s", devent_errno());
      return NULL;
    }
#else
    fd = WSASocketW(address->sa_family, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (fd == INVALID_SOCKET) {
      LOGD("docket_create_listener failed: fd = -1, reason = %s", devent_errno());
      return NULL;
    }
#endif
  }

  // reuse address
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *) address, socklen);

  if (bind(fd, address, socklen) == -1) {
    LOGD("docket_create_listener failed: bind = -1, reason = %s", devent_errno());
    return NULL;
  }

  if (listen(fd, DOCKET_LISTENER_BACKLOG) == -1) {
    LOGD("docket_create_listener failed: bind = -1, reason = %s", devent_errno());
    return NULL;
  }

  DocketListener *listener = calloc(1, sizeof(DocketListener));
  listener->fd = fd;
  listener->cb = cb;
  listener->ctx = ctx;

#ifndef WIN32
  devent_turn_on_flags(fd, O_NONBLOCK);
  devent_update_events(docket->fd, fd, DEVENT_READ_WRITE, 0);
#else
  CreateIoCompletionPort((HANDLE) fd, docket->fd, fd, 0);

  SOCKET acceptSocket = WSASocketW(address->sa_family, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
  IO_CONTEXT *io = IO_CONTEXT_new(IOCP_OP_ACCEPT, acceptSocket);
  io->local_len = socklen;
  io->remote_len = socklen;

  docket_accept(docket, fd, acceptSocket, io);
#endif

#ifdef DEVENT_SSL
  listener->ssl_ctx = ssl_ctx;
  if (ssl_ctx) {
    SSL_CTX_set_cert_verify_callback(ssl_ctx, cert_verify_callback, ctx);
  }
#endif

  Docket_add_listener(docket, listener);
  return listener;
}

#ifdef DEVENT_SSL

DocketListener *
Docket_create_ssl_listener(Docket *docket,
                           SSL_CTX *ssl_ctx,
                           int (*cert_verify_callback)(X509_STORE_CTX *, void *),
                           docket_connect_cb cb,
                           int fd,
                           struct sockaddr *address,
                           socklen_t socklen,
                           void *ctx) {
  return Docket_create_listener_internal(docket, ssl_ctx, cert_verify_callback, cb, fd, address, socklen, ctx);
}

DocketListener *
Docket_create_ssl_listener_by_path(Docket *docket, const char *cert_path, const char *key_path,
                                   int (*cert_verify_callback)(X509_STORE_CTX *, void *),
                                   docket_connect_cb cb,
                                   int fd, struct sockaddr *address,
                                   socklen_t socklen, void *ctx) {
  SSL_library_init();
  ERR_load_crypto_strings();
  SSL_load_error_strings();
  OpenSSL_add_all_algorithms();

  const SSL_METHOD *method;
  method = SSLv23_server_method();
  SSL_CTX *ssl_ctx = SSL_CTX_new(method);

  if (ssl_ctx == NULL) {
    LOGE("SSL_CTX_new failed.");
    ERR_print_errors_fp(stderr);
    return NULL;
  }

  SSL_CTX_set_ecdh_auto(ssl_ctx, 1);

  if (SSL_CTX_use_certificate_file(ssl_ctx, cert_path, SSL_FILETYPE_PEM) <= 0) {
    LOGE("SSL_CTX_use_certificate_file failed.");
    ERR_print_errors_fp(stderr);
    return NULL;
  }

  if (SSL_CTX_use_PrivateKey_file(ssl_ctx, key_path, SSL_FILETYPE_PEM) <= 0) {
    LOGE("SSL_CTX_use_PrivateKey_file failed.");
    ERR_print_errors_fp(stderr);
    return NULL;
  }

  return Docket_create_listener_internal(docket, ssl_ctx, cert_verify_callback, cb, fd, address, socklen, ctx);
}

#endif

DocketListener *
Docket_create_listener(Docket *docket, docket_connect_cb cb, int fd, struct sockaddr *address, socklen_t socklen,
                       void *ctx) {
  return Docket_create_listener_internal(docket,
#ifdef DEVENT_SSL
      NULL, NULL,
#endif
                                         cb, fd, address, socklen, ctx);
}
