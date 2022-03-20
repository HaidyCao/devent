//
// Created by Haidy on 2021/11/20.
//
#include <stdio.h>
#ifndef WIN32
#include <strings.h>
#endif

#include "echo_server_test.h"

#include "clib.h"
#include "docket.h"
#include "event.h"
#include "listener.h"
#include "utils.h"

#define LOGD(fmt, ...) printf("[%s(%d):%s]: " fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

static void echo_read(DocketEvent *event, void *ctx) {
  char data[1024];
  bzero(data, sizeof(data));
  ssize_t len = DocketEvent_read(event, data, sizeof(data));
  if (len == 0) {
    LOGD("no data to read");
    return;
  }

  if (len == -1) {
    LOGD("read data failed: %s", devent_errno());
    return;
  }
  printf("read: %s\n", data);

  // write origin data to client
  DocketEvent_write(event, data, len);
}

static void on_listen(DocketListener *l, SOCKET fd, struct sockaddr *address,
                      socklen_t address_len, DocketEvent *event, void *ctx) {
  LOGD("on connect: address = %s", sockaddr_to_string(address, NULL, 0));
  DocketEvent_set_read_cb(event, echo_read, NULL);
}

static void echo() {
  printf("Hello, World!\n");

  Docket *docket = Docket_new();

  struct sockaddr_storage local;
  socklen_t socklen = sizeof(local);
  const char *address = "0.0.0.0:1188";
  if (parse_address(address, (struct sockaddr *) &local, &socklen) == -1) {
    LOGD("parse address failed: %s", address);
    return;
  }

  Docket_create_listener(docket, on_listen, -1, (struct sockaddr *) &local,
                         socklen, docket);
  Docket_loop(docket);

  LOGD("Docket_loop end");
}

void echo_server_start() { echo(); }