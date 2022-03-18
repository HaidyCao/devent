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

#ifdef DEVENT_SSL
#include "openssl/ssl.h"
#endif

#ifdef WIN32
void docket_on_handle_read_data(IO_CONTEXT *io, DWORD size, DocketEvent *event) {
  DocketBuffer_write(event->in_buffer, io->buf.buf, size);

  if (devent_read_enable(event) && event->read_cb) {
    event->read_cb(event, event->ctx);
  }
}
#else
void docket_on_event_read(DocketEvent *event) {
  LOGD("");
  set_write_disable(event);
  static __thread char buf[DEVENT_READ_BUFFER_SIZE];

  int fd = event->fd;
  Docket *docket = event->docket;

  if (!devent_read_enable(event)) {
    devent_update_events(docket->fd, fd, event->ev, 1);
    return;
  }

  if (!devent_do_ssl_handshake(event)) {
    return;
  }

  ssize_t len;
  struct sockaddr_storage address;
  socklen_t socklen = sizeof(address);
  int err_number = 0;

  do {
#ifdef DEVENT_SSL
    if (event->ssl) {
      len = SSL_read(event->ssl, buf, DEVENT_READ_BUFFER_SIZE);
    } else
#endif

    if (event->remote_address) {
      bzero(&address, sizeof(address));
      len = recvfrom(fd, buf, DEVENT_READ_BUFFER_SIZE, 0, (struct sockaddr *) &address, &socklen);
      LOGD("recvfrom: length = %d", len);
    } else {
      len = read(fd, buf, DEVENT_READ_BUFFER_SIZE);
      LOGD("fd = %d, read: length = %d", event->fd, len);
    }

    if (len <= 0) {
      err_number = errno;
      break;
    }

    if (event->remote_address) {
      // TODO udp
    } else {
      DocketBuffer_write(event->in_buffer, buf, len);
      if (event->read_cb) {
        event->read_cb(event, event->ctx);
      }
    }

    event = Docket_find_event(docket, fd);
    if (event == NULL) {
      LOGD("event is freed: fd = %d", fd);
      return;
    }
  } while (devent_read_enable(event));

  if (len > 0 || (len == -1 && (errno_is_EAGAIN(err_number)))) {
    // TODO update read and write timeout
    set_write_enable(event);
    devent_write_data(event, event->out_buffer, NULL, 0);
    return;
  }

  // do final read callback
  if (DocketBuffer_length(event->in_buffer) > 0 && event->read_cb) {
    event->read_cb(event, event->ctx);
  }

  event = Docket_find_event(docket, fd);
  if (event) {
    devent_close_internal(event, DEVENT_READ_EOF);
  }
}
#endif