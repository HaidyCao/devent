//
// Created by Haidy on 2021/12/5.
//

#include "ssl_telnet_test.h"
#include "docket.h"
#include "connect.h"
#include "file_event.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
#include <WinSock2.h>
#include "event.h"
#else
#include <string.h>
#endif
#define LOGD(fmt, ...) printf("[%s(%d):%s]: " fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

static void on_stdin_read(DocketEvent *ev, void *ctx) {
  DocketEvent *remove_ev = ctx;

  char buffer[1024 * 10];
  ssize_t len = DocketEvent_read(ev, buffer, sizeof(buffer));
  if (len > 0) {
    if (len >= 2 && buffer[len - 1] == '\n' && buffer[len - 2] == '\r') {
      buffer[len - 1] = '\r';
      buffer[len] = '\n';
      buffer[len + 1] = '\0';
    } else {
      buffer[len] = '\0';
    }
    DocketEvent_write(remove_ev, buffer, strlen(buffer));
  }
}

static void on_remote_read(DocketEvent *ev, void *ctx) {
  char buffer[1024 * 10];
  ssize_t len = DocketEvent_read(ev, buffer, sizeof(buffer));

  if (len > 0) {
    fwrite(buffer, 1, len, stdout);
    fwrite("\n", 1, 1, stdout);
  }
}

static void on_connect(DocketEvent *ev, int what, void *ctx) {
  Docket *docket = ctx;
  if (what & DEVENT_ERROR) {
    if (what & DEVENT_CONNECT) {
      LOGD("connect failed");
    } else if (what & DEVENT_WRITE) {
      LOGD("write error");
    }
    exit(-1);
  }

  DocketEvent_set_read_cb(ev, on_remote_read, NULL);

  DocketEvent *stdin_event = Docket_create_stdin_event(docket);
  DocketEvent_set_read_cb(stdin_event, on_stdin_read, ev);
}

void ssl_telnet_start() {
  Docket *docket = Docket_new();

//  DocketEventSSL *event = DocketEvent_connect_hostname_ssl(docket, -1, "imap.qq.com", 993);
  DocketEvent *event = DocketEvent_connect_hostname_ssl(docket, -1, "192.168.31.151", 1234);

  DocketEvent_set_event_cb(event, on_connect, docket);

  Docket_loop(docket);
}