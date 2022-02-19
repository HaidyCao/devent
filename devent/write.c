//
// Created by Haidy on 2021/11/6.
//

#include <stdbool.h>
#include <errno.h>

#include "write.h"
#include "utils_internal.h"
#include "log.h"
#include "docket.h"
#include "docket_def.h"
#include "event_def.h"
#include "win_def.h"

void devent_write_data(DocketEvent *event, DocketBuffer *buffer, struct sockaddr *address, socklen_t socklen) {
  int fd = event->fd;
  LOGD("fd = %d", fd);
  Docket *docket = event->docket;

#ifdef WIN32
  int flags = 0;
#else
  int flags = MSG_NOSIGNAL;
#endif
  bool write_enable = devent_write_enable(event);
  if (write_enable && DocketBuffer_length(buffer) > 0) {
    while (devent_write_enable(event) && DocketBuffer_length(buffer) > 0) {

      ssize_t wr = DocketBuffer_send(buffer, fd, flags, address, socklen
#ifdef DEVENT_SSL
                                     , event->ssl
#endif
                                     );
      if (wr == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          set_write_disable(event);
          devent_update_events(docket->fd, fd, event->ev, 0);
        } else {
          LOGE("write_data: unknown error: %s", devent_errno());
          devent_close_internal(event, DEVENT_WRITE | DEVENT_ERROR);
          return;
        }
      } else if (wr == 0) {
        devent_close_internal(event, DEVENT_WRITE | DEVENT_ERROR);
        return;
      } else {
        LOGD("write length: %d", wr);
      }
    }
  } else if (!write_enable) {
    set_write_enable(event);
    devent_update_events(docket->fd, fd, event->ev, 0);
  }

  if (event->write_cb) {
    event->write_cb(event, event->ctx);
  }
}

#ifdef WIN32
void docket_on_event_write(DocketEvent *event) {
  if (!devent_write_enable(event)) {
    return;
  }

  IO_CONTEXT *io = IO_CONTEXT_new(IOCP_OP_READ, event->fd);
  // TODO SSL


}
#else
void docket_on_event_write(DocketEvent *event) {
  int fd = DocketEvent_getFD(event);

  LOGD("event fd = %d", fd);

  // disable write
  set_write_disable(event);
  devent_update_events(event->docket->fd, fd, event->ev, 1);

  // ssl handshaking
#ifdef DEVENT_SSL
  if (!devent_do_ssl_handshake(event)) {
    return;
  }
#endif

  // write enable
  set_write_enable(event);
  if (DocketBuffer_length(event->out_buffer) == 0) {
    // no date to write
    return;
  }

  // TODO UDP
  devent_write_data(event, event->out_buffer, event->remote_address, event->remote_address_len);
}
#endif