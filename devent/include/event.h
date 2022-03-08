//
// Created by Haidy on 2021/9/25.
//

#ifndef DOCKET_EVENT_H
#define DOCKET_EVENT_H

#ifndef _MSC_VER
#include <sys/socket.h>
#endif
#include <stdlib.h>
#include <stdbool.h>

#include "buffer.h"
#include "def.h"
#include "../win_def.h"

#define DEVENT_NONE 0
#define DEVENT_READ 1
#define DEVENT_WRITE 2
#define DEVENT_READ_WRITE DEVENT_READ | DEVENT_WRITE
#define DEVENT_CONNECT 4
#define DEVENT_ERROR 8
#define DEVENT_EOF 16
#define DEVENT_OPENSSL 32

#define DEVENT_READ_EOF DEVENT_READ | DEVENT_EOF

#define DEVENT_IS_ERROR_OR_EOF(what) ((what & DEVENT_ERROR) || (what & DEVENT_EOF))

#define DEVENT_TYPE_TCP 1
#define DEVENT_TYPE_UDP 2

typedef void (*docket_event_callback)(DocketEvent *ev, int what, void *ctx);

typedef void (*docket_event_read_callback)(DocketEvent *ev, void *ctx);

typedef void (*docket_event_write_callback)(DocketEvent *ev, void *ctx);

/**
 *
 */
DocketEvent *DocketEvent_new(Docket *docket, SOCKET fd, void *ctx);

Docket *DocketEvent_getDocket(DocketEvent *event);

DocketBuffer *DocketEvent_get_in_buffer(DocketEvent *event);

DocketBuffer *DocketEvent_get_out_buffer(DocketEvent *event);

void DocketEvent_set_read_cb(DocketEvent *event, docket_event_read_callback cb, void *ctx);

void DocketEvent_set_write_cb(DocketEvent *event, docket_event_write_callback cb, void *ctx);

void DocketEvent_set_event_cb(DocketEvent *event, docket_event_callback cb, void *ctx);

void devent_set_read_enable(DocketEvent *event, bool enable);

void
DocketEvent_set_cb(DocketEvent *event, docket_event_read_callback read_cb, docket_event_write_callback write_cb,
                   docket_event_callback event_cb,
                   void *ctx);

/**
 * write data to
 * @param event event
 * @param data  data
 * @param len   length of data
 */
void DocketEvent_write(DocketEvent *event, const char *data, size_t len);

/**
 * write buffer to
 * @param event     event
 * @param buffer    buffer
 */
void DocketEvent_write_buffer(DocketEvent *event, DocketBuffer *buffer);

/**
 * force write buffer to
 * @param event     event
 * @param buffer    buffer
 */
void DocketEvent_write_buffer_force(DocketEvent *event, DocketBuffer *buffer);

/**
 * read data from event
 * @param event event
 * @param data  data
 * @param len   length of data
 * @return length of data read from event, or -1 if read failed
 */
ssize_t DocketEvent_read(DocketEvent *event, char *data, size_t len);

/**
 * read ${len} data from event
 * @param event event
 * @param data  data
 * @param len   length of data
 * @return length of data read from event, or -1 if read failed
 */
ssize_t DocketEvent_read_full(DocketEvent *event, char *data, size_t len);

/**
 * peek input data
 */
ssize_t DocketEvent_peek(DocketEvent *event, char *data, size_t len);

/**
 * peek input data
 */
ssize_t DocketEvent_peek_full(DocketEvent *event, char *data, size_t len);

bool DocketEvent_fetch_local_address(DocketEvent *event, struct sockaddr *address, socklen_t *socklen,
                                     unsigned short *port);

void DocketEvent_free(DocketEvent *event);

#endif //DOCKET_EVENT_H
