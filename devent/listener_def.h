//
// Created by Haidy on 2021/12/5.
//

#ifndef DOCKET_DEVENT_LISTENER_DEF_H
#define DOCKET_DEVENT_LISTENER_DEF_H

#include "listener.h"

struct docket_listener {
  SOCKET fd;
  docket_connect_cb cb;
  void *ctx;

#ifdef DEVENT_SSL
  SSL_CTX *ssl_ctx;
#endif
};

#endif //DOCKET_DEVENT_LISTENER_DEF_H
