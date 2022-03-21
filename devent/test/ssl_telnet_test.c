//
// Created by Haidy on 2021/12/5.
//

#include "ssl_telnet_test.h"
#include "docket.h"
#include "connect.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
#include <WinSock2.h>
#else
#include <string.h>
#endif
#define LOGD(fmt, ...) printf("[%s(%d):%s]: " fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

static void on_stdin_read(DocketEvent *ev, void *ctx) {
  LOGD("");
  DocketEvent *remove_ev = ctx;

  DocketBuffer *buf = DocketEvent_get_in_buffer(ev);

  char buffer[1024 * 10];
  ssize_t len = DocketBuffer_read(buf, buffer, sizeof(buffer));
  if (len > 0) {
    fwrite(buffer, 1, len, stdout);
  }
}

static void on_remote_read(DocketEventSSL *ev, void *ctx) {
  char buffer[1024 * 10];
  ssize_t len = DocketEventSSL_read(ev, buffer, sizeof(buffer));

  if (len > 0) {
    fwrite(buffer, 1, len, stdout);

    scanf("%s", buffer);
    DocketEventSSL_write(ev, buffer, strlen(buffer));
  }
}

static void on_connect(DocketEventSSL *ev, int what, void *ctx) {
  if (what & DEVENT_ERROR) {
    LOGD("connect failed");
    exit(-1);
  }

//  DocketEvent_set_cb(ev, on_remote_read, NULL, on_connect, NULL);

  DocketEvent_set_ssl_read_cb(ev, on_remote_read, NULL);
//  int file = CreateFile("CONIN$", FILE_GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);

  char buffer[1024 * 10];
  scanf("%s", buffer);
  DocketEventSSL_write(ev, buffer, strlen(buffer));
}

void ssl_telnet_start() {
  Docket *docket = Docket_new();
//  Docket_set_dns_server(docket, "114.114.114.114");

  DocketEventSSL *event = DocketEvent_connect_hostname_ssl(docket, -1, "imap.qq.com", 993);
//  DocketEventSSL *event = DocketEvent_connect_hostname_ssl(docket, -1, "127.0.0.1", 1234);

//  DocketEvent_set_event_cb(event, on_connect, docket);
  DocketEvent_set_ssl_event_cb(event, on_connect, docket);
  Docket_loop(docket);
}