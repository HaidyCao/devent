//
// Created by Haidy on 2021/11/6.
//

#ifndef WIN32
#include <errno.h>
#endif

#include "write.h"
#include "utils_internal.h"
#include "log.h"
#include "docket.h"
#include "event.h"
#include "event_def.h"
#include "buffer.h"

ssize_t devent_write_data(DocketEvent *event, DocketBuffer *buffer) {
  SOCKET fd = event->fd;

  if (DocketBuffer_length(buffer) > 0) {
#ifdef WIN32
    if (devent_write_enable(event)) {
      ssize_t send_result = DocketBuffer_send(event, fd, 0, buffer);
      if (send_result <= 0) {
        LOGE("DocketBuffer_send failed: %s", devent_errno());
        DocketEvent_free(event);
        return -1;
      }
      return send_result;
    }
    return -1;
#else
    Docket *docket = event->docket;
    int flags = MSG_NOSIGNAL;
    ssize_t total = 0;
    while (event && devent_write_enable(event) && DocketBuffer_length(buffer) > 0) {

      ssize_t wr = DocketBuffer_send(event, fd, flags, buffer);
      if (wr == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          // system no buffer for wait, wait more
          devent_update_events(docket->fd, fd, event->ev | DEVENT_WRITE, DEVENT_MOD_MOD);
          if (total == -1) {
            total = 0;
          }
          break;
        } else {
          LOGE("write_data: unknown error: %s", devent_errno());
          devent_close_internal(event, DEVENT_WRITE | DEVENT_ERROR);
          return -1;
        }
      } else if (wr == 0) {
        devent_close_internal(event, DEVENT_WRITE | DEVENT_EOF);
        return -1;
      } else {
        LOGD("fd = %d: write length: %d", fd, wr);
        if (total == -1) {
          total = 0;
        }
        total += wr;

        if (event->write_cb) {
          event->write_cb(event, event->ctx);
          event = Docket_find_event(docket, fd);
        }
      }
    }
    return total;
#endif
  }
  return -1;
}

void docket_on_event_write(DocketEvent *event) {
  if (!devent_write_enable(event)) {
    return;
  }

//  devent_update_events(event->docket->fd, event->fd, event->ev & (~DEVENT_WRITE), DEVENT_MOD_MOD);
  devent_write_data(event, event->out_buffer);
}