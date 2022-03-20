//
// Created by Haidy on 2021/12/5.
//

#include "ssl_echo_server_test.h"

#include "docket.h"
#include "listener.h"
#include "clib.h"
#include "log.h"
#include "event.h"
#include "utils.h"

static void echo_read(DocketEvent *event, void *ctx) {
  char data[1024];
  ssize_t len = DocketEvent_read(event, data, sizeof(data));
  if (len == 0) {
    LOGI("no data to read");
    return;
  }

  if (len == -1) {
    LOGE("read data failed: %s", devent_errno());
    return;
  }

  // write origin data to client
  DocketEvent_write(event, data, len);
}

static void on_listen(DocketListener *l, int fd, struct sockaddr *address, socklen_t address_len,
                      DocketEvent *event,
                      void *ctx) {
  LOGD("on connect: address = %s", sockaddr_to_string(address, NULL, 0));
  DocketEvent_set_read_cb(event, echo_read, NULL);
}

int cert_verify_callback(X509_STORE_CTX *store_ctx, void *ctx) {
  LOGD("-----------------------------------------");
  return 1;
}

static void echo() {
  Docket *docket = Docket_new();

  struct sockaddr_storage local;
  socklen_t socklen = sizeof(local);
  const char *address = "127.0.0.1:1188";
  if (parse_address(address, (struct sockaddr *) &local, &socklen) == -1) {
    LOGE("parse address failed: %s", address);
    return;
  }

  Docket_create_ssl_listener_by_path(docket,
                                     "/Users/haidy/AndroidStudioProjects/Docket/app/src/main/cpp/devent/test/full_chain.pem",
                                     "/Users/haidy/AndroidStudioProjects/Docket/app/src/main/cpp/devent/test/private.key",
                                     cert_verify_callback,
                                     on_listen,
                                     -1,
                                     (struct sockaddr *) &local,
                                     socklen,
                                     docket);
  Docket_loop(docket);
}

void echo_ssl_server_start() {
  echo();
}