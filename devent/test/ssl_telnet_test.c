//
// Created by Haidy on 2021/12/5.
//

#include "ssl_telnet_test.h"
#include "docket.h"
#include "connect.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static char buffer[1024 * 1024 * 1024];

static void on_stdin_read(DocketEvent *ev, void *ctx) {
  LOGD("");
  DocketEvent *remove_ev = ctx;

  DocketBuffer *buf = DocketEvent_get_in_buffer(ev);

  ssize_t len = DocketBuffer_read(buf, buffer, sizeof(buffer));
  if (len > 0) {
    write(STDOUT_FILENO, buffer, len);
    DocketEvent_write(remove_ev, buffer, len);
    DocketEvent_write(remove_ev, "\r\n", 2);
  }
}

static void on_remote_read(DocketEvent *ev, void *ctx) {
  DocketBuffer *buf = DocketEvent_get_in_buffer(ev);
  ssize_t len = DocketBuffer_read(buf, buffer, sizeof(buffer));

  if (len > 0) {
    write(STDOUT_FILENO, buffer, len);
  }
}

static void on_connect(DocketEvent *ev, int what, void *ctx) {
  Docket *docket = ctx;
  LOGD("what = %d", what);
  if (what & DEVENT_ERROR) {
    LOGE("connect failed");
    exit(-1);
  }

  DocketEvent_set_cb(ev, on_remote_read, NULL, on_connect, NULL);

  DocketEvent *ev_stdin = DocketEvent_create(docket, STDIN_FILENO);
  DocketEvent_set_read_cb(ev_stdin, on_stdin_read, ev);
}

void ssl_telnet_start() {
  Docket *docket = Docket_new();
  Docket_set_dns_server(docket, "114.114.114.114");

//  DocketEvent *event = DocketEvent_connect_hostname_ssl(docket, -1, "imap.qq.com", 993);

#ifdef DEVENT_SSL
  DocketEvent *event = DocketEvent_connect_hostname_ssl(docket, -1, "127.0.0.1", 1188);
#else
  DocketEvent *event = DocketEvent_connect_hostname(docket, -1, "127.0.0.1", 1188);
#endif

  DocketEvent_set_event_cb(event, on_connect, docket);
  Docket_loop(docket);
}