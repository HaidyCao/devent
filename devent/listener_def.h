//
// Created by Haidy on 2021/12/5.
//

#ifndef DOCKET_DEVENT_LISTENER_DEF_H
#define DOCKET_DEVENT_LISTENER_DEF_H

#include <stdbool.h>
#include "listener.h"

struct docket_listener {
  SOCKET fd;
  docket_accept_cb cb;
  // will be DocketSSLListenerContext in ssl
  void *ctx;

#ifdef DEVENT_SSL
  void *ssl_arg;
  SSL_CTX *ssl_ctx;
#endif
};

#ifdef DEVENT_SSL
typedef struct {
  SSL_CTX *ssl_ctx;
  docket_accept_cb cb;
  void *ctx;
} DocketSSLListenerContext;
#endif

#endif //DOCKET_DEVENT_LISTENER_DEF_H
