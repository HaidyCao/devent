//
// Created by Haidy on 2021/11/20.
//
#include <stdio.h>
#include <string.h>

#include "telnet_test.h"
#include "docket.h"
#include "event.h"
#include "clib.h"
#include "connect.h"

#define LOGD(fmt, ...) printf("[%s(%d):%s]: " fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

static char buffer[1024 * 1024 * 1024];

static void read_cb(DocketEvent *ev, void *ctx) {
    ssize_t len = DocketEvent_read(ev, buffer, sizeof(buffer));
    if (len > 0) {
        buffer[len] = '\0';
        printf("%s\n", buffer);
    }

    scanf("%s", buffer);
    DocketEvent_write(ev, buffer, strlen(buffer));
}

static void on_connect(DocketEvent *ev, int what, void *ctx) {
    LOGD("what = %d", what);
    if (what & DEVENT_ERROR) {
        LOGD("connect failed");
        exit(-1);
    }

    DocketEvent_set_cb(ev, read_cb, NULL, on_connect, NULL);

    scanf("%s", buffer);
    DocketEvent_write(ev, buffer, strlen(buffer));
}

void telnet_start() {
    Docket *docket = Docket_new();
//    Docket_set_dns_server(docket, "114.114.114.114");

    socklen_t addr_len = sizeof(struct sockaddr_storage);
    struct sockaddr_storage sockaddr;
    parse_address("127.0.0.1:1188", (struct sockaddr *)(&sockaddr), &addr_len);

//    DocketEvent *event = DocketEvent_connect(docket, -1, (struct sockaddr *)&sockaddr, addr_len);
    DocketEvent *event = DocketEvent_connect_hostname(docket, -1, "www.baidu.com", 80);
    DocketEvent_set_event_cb(event, on_connect, NULL);
    Docket_loop(docket);
}