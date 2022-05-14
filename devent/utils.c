//
// Created by Haidy on 2021/10/31.
//

#include <string.h>
#include <stdio.h>
#include <errno.h>

#ifdef WIN32
#include <WinSock2.h>
#else
#include <fcntl.h>
#include <strings.h>
#endif
#include "utils_internal.h"
#include "event.h"
#include "event_def.h"
#include "log.h"
#include "docket.h"
#include "docket_def.h"
#include "listener_def.h"
#include "event_ssl.h"

char *devent_errno() {
  static char msg[1024];
  bzero(&msg, sizeof(msg));
#ifdef WIN32
  char lastErrorMsg[1024];
  bzero(&lastErrorMsg, sizeof(lastErrorMsg));

  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                WSAGetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                msg,
                sizeof(msg),
                NULL);

  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                lastErrorMsg,
                sizeof(lastErrorMsg),
                NULL);

  sprintf(msg,
          "WSALastErrorCode = %d, WSALastError = %s, LastError = %lu, LastError = %s",
          WSAGetLastError(),
          msg,
          GetLastError(),
          lastErrorMsg);
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
#elif __linux__ || __ANDROID__
#include <sys/epoll.h>
#endif

int devent_update_events(
#ifdef WIN32
    HANDLE handle,
#else
    int dfd,
#endif
    SOCKET fd, unsigned int events, int mod) {
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
  struct epoll_event event;
  bzero(&event, sizeof(event));
  event.data.fd = fd;

  int op = 0;
  if (mod == DEVENT_MOD_MOD) {
    op = EPOLL_CTL_MOD;
  } else if (mod == DEVENT_MOD_ADD) {
    op = EPOLL_CTL_ADD;
  } else if (mod == DEVENT_MOD_DEL) {
    op = EPOLL_CTL_DEL;
  }

  if (events & DEVENT_READ) {
    event.events = EPOLLIN;
  }

  if (events & DEVENT_WRITE) {
    event.events |= EPOLLOUT;
  }
  LOGD("update event: fd = %d, readable = %d, writeable = %d",
       fd,
       (event.events & EPOLLIN) != 0,
       (event.events & EPOLLOUT) != 0);

  if (events & DEVENT_ET) {
    event.events |= EPOLLET;
  }

  if (epoll_ctl(dfd, op, fd, &event) == -1) {
    LOGD("epoll_ctl failed: errno = %d, errmsg: %s", errno, strerror(errno));
    return -1;
  }
  return 0;
#elif WIN32
  return 0;
#endif
}

void devent_close_internal(DocketEvent *event, int what) {
  LOGD("devent_close: event fd = %d, what = %d", event->fd, what);
  Docket *docket = event->docket;
  SOCKET fd = event->fd;

  event->ev = DEVENT_NONE;

#ifdef DEVENT_SSL
  if (event->ssl) {
    DocketEventSSLContext *ssl_context = event->ctx;
    if (ssl_context && ssl_context->ssl_event_cb) {
      ssl_context->ssl_event_cb(event, what, ssl_context->ssl_ctx);
    }
  } else
#endif
  {
    if (event->event_cb) {
      event->event_cb(event, what, event->ctx);
    }
  }

  DocketEvent_free(Docket_find_event(docket, fd));
}

bool errno_is_EAGAIN(int eno) {
  if (eno == EAGAIN) {
    return true;
  }

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

char *docket_memdup(const char *data, unsigned int len) {
  if (data == NULL) {
    return NULL;
  }
  char *mem = malloc(len);
  if (mem == NULL) {
    return NULL;
  }
  memcpy(mem, data, len);
  return mem;
}