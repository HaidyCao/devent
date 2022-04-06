//
// Created by Haidy on 2021/9/25.
//

#include <string.h>
#include <errno.h>
#ifdef WIN32
#include <WS2tcpip.h>
#else
#include <unistd.h>
#endif

#include "read.h"
#include "log.h"
#include "write.h"
#include "utils_internal.h"
#include "docket_def.h"
#include "event_def.h"
#include "listener_def.h"
#include "win_def.h"
#include "event_def.h"
#include "event.h"
#include "docket.h"

#ifdef WIN32
void docket_on_handle_read_data(IO_CONTEXT *io, DWORD size, DocketEvent *event) {
  DocketBuffer_write(event->in_buffer, io->buf.buf, size);

  if (devent_read_enable(event) && event->read_cb) {
    event->read_cb(event, event->ctx);
  }
}
#else

#define DEVENT_READ_BUFFER_SIZE 10240

void docket_on_event_read(DocketEvent *event) {
  static __thread char buf[DEVENT_READ_BUFFER_SIZE];

  int fd = event->fd;
  Docket *docket = event->docket;

  if (!devent_read_enable(event)) {
    return;
  }
//  devent_update_events(docket->fd, fd, event->ev & (~DEVENT_READ), DEVENT_MOD_MOD);

  ssize_t len;
  struct sockaddr_storage address;
  socklen_t socklen = sizeof(address);
  int err_number = 0;

  do {
    if (event->remote_address) {
      bzero(&address, sizeof(address));
      len = recvfrom(fd, buf, DEVENT_READ_BUFFER_SIZE, 0, (struct sockaddr *) &address, &socklen);
      LOGD("recvfrom: length = %d", len);
    } else {
      len = read(fd, buf, DEVENT_READ_BUFFER_SIZE);
      LOGD("fd = %d, read: length = %d", event->fd, len);

      if (len == 0) {
        LOGD("errno = %s", devent_errno());
      }
    }

    if (len <= 0) {
      err_number = errno;
      break;
    }

    DocketBuffer_write(event->in_buffer, buf, len);
    if (event->read_cb) {
      event->read_cb(event, event->ctx);
    }

    event = Docket_find_event(docket, fd);
    if (event == NULL) {
      LOGD("event is freed: fd = %d", fd);
      return;
    }
  } while (devent_read_enable(event));

  if (len > 0 || (len == -1 && errno_is_EAGAIN(err_number)) || (event->read_zero_enable && len == 0)) {
    // TODO update read and write timeout
//    devent_update_events(docket->fd, event->fd, event->ev | DEVENT_READ, DEVENT_MOD_MOD);

    if (DocketBuffer_length(event->out_buffer) > 0) {
      devent_write_data(event, event->out_buffer);
    }
    return;
  }

  // do final read callback and then try close event
  if (DocketBuffer_length(event->in_buffer) > 0 && event->read_cb) {
    event->read_cb(event, event->ctx);
  }

  event = Docket_find_event(docket, fd);
  if (event) {
    devent_close_internal(event, DEVENT_READ_EOF);
  }
}
#endif