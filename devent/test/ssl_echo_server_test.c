//
// Created by Haidy on 2021/12/5.
//
#include <stdio.h>
#include <string.h>
#include "ssl_echo_server_test.h"

#include "docket.h"
#include "listener.h"
#include "clib.h"
#include "event.h"
#include "utils.h"

#define LOGD(fmt, ...) printf("[%s(%d):%s]: " fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define LOGI(fmt, ...) printf("[%s(%d):%s]: " fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define LOGE(fmt, ...) printf("[%s(%d):%s]: " fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

#ifdef WIN32
#define PATH_MAX 1024
#endif

static char *file_path(const char *file_name) {
  char path[PATH_MAX];
  const char *cul_file_name = __FILE__;
#ifdef WIN32
  char *end = strrchr(cul_file_name, '\\');
#else
  char *end = strrchr(cul_file_name, '/');
#endif
  if (end == NULL) {
    return NULL;
  }

  bzero(path, sizeof(path));
  strcpy(path, cul_file_name);

  strcpy(path + (end - cul_file_name) + 1, file_name);
  return strdup(path);
}

static void echo_event(DocketEvent *event, int what, void *ctx) {
  LOGE("");
  if (what & DEVENT_ERROR || what & DEVENT_EOF) {
    exit(0);
  }
}

static char buffer[4096];
static void echo_read(DocketEvent *event, void *ctx) {
  ssize_t len = DocketEvent_read(event, buffer, sizeof(buffer));
  if (len == 0) {
    LOGI("no data to read");
    return;
  }

  if (len == -1) {
    LOGE("read data failed: %s", devent_errno());
    return;
  }
  buffer[len] = '\0';

  LOGD("%s", buffer);

  // write origin data to client
  DocketEvent_write(event, buffer, len);
}

static void on_listen(DocketListener *l, SOCKET fd, struct sockaddr *address, socklen_t address_len,
                      DocketEvent *event,
                      void *ctx) {
  LOGD("on connect: address = %s", sockaddr_to_string(address, NULL, 0));
  DocketEvent_set_read_cb(event, echo_read, NULL);
  DocketEvent_set_event_cb(event, echo_event, NULL);
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
//                                     ("C:\\Users\\haidy\\CLionProjects\\devent\\dependencies\\linux\\certs\\full_chain.pem"),
//                                     ("C:\\Users\\haidy\\CLionProjects\\devent\\dependencies\\linux\\certs\\private.key"),
                                     file_path("cert.pem"),
                                     file_path("key.pem"),
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