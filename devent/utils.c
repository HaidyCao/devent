//
// Created by Haidy on 2021/10/31.
//

#include <string.h>
#include <stdio.h>
#include <errno.h>

#ifdef WIN32
#include <WinSock2.h>
#endif
#include "utils_internal.h"
#include "event.h"
#include "event_def.h"
#include "log.h"
#include "docket.h"
#include "docket_def.h"
#include "listener_def.h"


char *devent_errno() {
  static char msg[1024];
  bzero(&msg, sizeof(msg));
#ifdef WIN32
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                WSAGetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                msg,
                sizeof(msg),
                NULL);
  sprintf(msg, "lastError = %d, WSALastError = %s", WSAGetLastError(), msg);
#else
  sprintf(msg, "errno = %d, errmsg: %s", errno, strerror(errno));
#endif
  return msg;
}

int devent_turn_on_flags(int fd, int flags) {
#ifndef WIN32
  int current_flags;
  // 获取给定文件描述符现有的flag
  // 其中fcntl的第二个参数F_GETFL表示要获取fd的状态
  if ((current_flags = fcntl(fd, F_GETFL)) < 0)
    return -1;

  // 施加新的状态位
  current_flags |= flags;
  if (fcntl(fd, F_SETFL, current_flags) < 0)
    return -1;
#endif
  return 0;
}

#ifdef __APPLE__

#include <sys/event.h>

#endif

/**
 * update event status
 * @param dfd docket fd
 * @param fd event fd
 * @param events event: read or write
 * @param mod 1 delete, 0 do nothing
 */
int devent_update_events(
#ifdef WIN32
    HANDLE handle,
#else
    int dfd,
#endif
    SOCKET fd, int events, int mod) {
#ifdef __APPLE__
  struct kevent ev[2];
  int n = 0;
  if (events & DEVENT_READ) {
    EV_SET(&ev[n++], fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, (void *) (intptr_t) fd);
  } else {
    EV_SET(&ev[n++], fd, EVFILT_READ, EV_DELETE, 0, 0, (void *) (intptr_t) fd);
  }
  if (events & DEVENT_WRITE) {
    EV_SET(&ev[n++], fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, (void *) (intptr_t) fd);
  } else {
    EV_SET(&ev[n++], fd, EVFILT_WRITE, EV_DELETE, 0, 0, (void *) (intptr_t) fd);
  }
  LOGD("%s fd %d events read %d write %d\n",
       mod ? "mod" : "add", fd, events & DEVENT_READ, events & DEVENT_WRITE);
  int r = kevent(dfd, ev, n, NULL, 0, NULL);
  if (r == -1) {
    LOGD("kevent failed: %s", devent_errno());
  }
  return r;
#elif __linux__ || __ANDROID__
  // TODO
#elif WIN32
  return 0;
#endif
}

void devent_close_internal(DocketEvent *event, int what) {
  LOGD("devent_close: event fd = %d, what = %d", event->fd, what);
  Docket *docket = event->docket;
  int fd = event->fd;

  event->ev = DEVENT_NONE;
  if (event->event_cb) {
    event->event_cb(event, what, event->ctx);
  }

  event = Docket_find_event(docket, fd);
  if (event) {
    DocketEvent_free(event);
  }
}

bool errno_is_EAGAIN(int eno) {
  if (eno == EAGAIN) {
    return true;
  }

#ifndef __APPLE__
  if (eno == EWOULDBLOCK) {
    return true;
  }
#endif
  return false;
}

bool devent_parse_ipv4(const char *ip, unsigned char *result) {
  if (ip == NULL) {
    return false;
  }

  int index = 0;
  char *left = (char *) ip;

  while (index < 4) {
    errno = 0;
    long a = strtol(left, &left, 10);
    if (errno == 0 && a >= 0 && a <= 255) {
      result[index] = (unsigned) a;
      index++;
      if (left[0] == '.') {
        left += 1;
      }
      continue;
    }
    return false;
  }
  return true;
}

bool devent_do_ssl_handshake(DocketEvent *event) {
  // TODO SSL on windows
#ifdef DEVENT_SSL
  if (event->ssl && event->ssl_handshaking) {
    int r = SSL_do_handshake(event->ssl);
    if (r != 1) {
      int err = SSL_get_error(event->ssl, r);
      if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
        LOGE("SSL error: %s", ERR_error_string(err, NULL));
        devent_close_internal(event, DEVENT_CONNECT | DEVENT_ERROR | DEVENT_OPENSSL);
      }
      return false;
    }
    event->ssl_handshaking = false;

    if (event->listener && event->listener->cb) {
      struct sockaddr_storage address;
      socklen_t socklen = sizeof(address);
      getpeername(event->fd, (struct sockaddr *) &address, &socklen);
      event->listener->cb(event->listener,
                          event->fd,
                          (struct sockaddr *) &address,
                          socklen,
                          event,
                          event->listener->ctx);
    }
  }
#endif
  return true;
}
