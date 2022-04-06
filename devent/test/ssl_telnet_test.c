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

//    sprintf(buffer, "%s\r\n", "A1 CAPABILITY");
//    DocketEventSSL_write(ev, buffer, strlen(buffer));

    scanf("%s", buffer);
    DocketEvent_write(ev, buffer, strlen(buffer));
  }
}

#ifdef WIN32
static char read_file_buf[4096];

static void test_io_completion_routine(
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransfered,
    LPOVERLAPPED lpOverlapped
) {

  LOGD("dwErrorCode %d, dwNumberOfBytesTransfered %d\n", dwErrorCode, dwNumberOfBytesTransfered);
}
#endif

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

//  DocketEvent_set_cb(ev, on_remote_read, NULL, on_connect, NULL);

  DocketEvent_set_read_cb(ev, on_remote_read, NULL);
//  int file = CreateFile("CONIN$", FILE_GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);

  char buffer[1024 * 10];
  scanf("%s", buffer);
  DocketEvent_write(ev, buffer, strlen(buffer));

//  DocketEvent *stdin_event = Docket_create_stdin_event(docket);
//  DocketEvent_set_read_cb(stdin_event, on_stdin_read, ev);

//  HANDLE file = CreateFile("CONIN$", FILE_GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
//  BindIoCompletionCallback(file, test_io_completion_routine, 0);
//
//  OVERLAPPED *olv = (OVERLAPPED *) malloc(sizeof(OVERLAPPED));
//  ZeroMemory(olv, sizeof(OVERLAPPED));
//  ReadFile(file, read_file_buf, sizeof(read_file_buf), NULL, olv);
}

void ssl_telnet_start() {
  Docket *docket = Docket_new();

//  DocketEventSSL *event = DocketEvent_connect_hostname_ssl(docket, -1, "imap.qq.com", 993);
  DocketEvent *event = DocketEvent_connect_hostname_ssl(docket, -1, "192.168.31.195", 1188);

  DocketEvent_set_event_cb(event, on_connect, docket);

//  DocketEvent *stdin_event = Docket_create_stdin_event(docket);
//  DocketEvent_set_read_cb(stdin_event, on_stdin_read, docket);
  Docket_loop(docket);
}