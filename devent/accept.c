//
// Created by Haidy on 2021/10/31.
//
#include "accept.h"
#include "log.h"
#include "utils_internal.h"

#ifdef WIN32
#include <MSWSock.h>
#include <WinSock2.h>

#include "event.h"
#include "event_def.h"

int docket_accept(Docket *docket, SOCKET listener, SOCKET fd, IO_CONTEXT *io) {
  DWORD dwBytes;
  GUID GuidAcceptEx = WSAID_ACCEPTEX;
  LPFN_ACCEPTEX lpfnAcceptEx = NULL;

  {
    int r = WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER,
                     &GuidAcceptEx, sizeof(GuidAcceptEx),
                     &lpfnAcceptEx, sizeof(lpfnAcceptEx),
                     &dwBytes, NULL, NULL);
    if (r == SOCKET_ERROR) {
      LOGD("accept failed: %s", devent_errno());
      closesocket(fd);
      return -1;
    }
  }
  io->lpOutputBuffer = malloc(256);

  BOOL r = lpfnAcceptEx(listener, fd, io->lpOutputBuffer, 0, io->local_len + 16, io->remote_len + 16, &dwBytes,
                        (LPOVERLAPPED) io);
  if (r == FALSE && WSAGetLastError() != WSA_IO_PENDING) {
    LOGD("accept failed: %s", devent_errno());
    closesocket(fd);
    return -1;
  }

  DocketEvent *event = DocketEvent_new(docket, fd, NULL);
  event->ev = DEVENT_READ_WRITE;

  // create event
  event->connected = true;
  Docket_add_event(event);

  return 0;
}

#else
#include <errno.h>
#include <sys/socket.h>
#include <sys/fcntl.h>

#include "utils_internal.h"
#include "listener_def.h"
#include "docket.h"
#include "event_def.h"
#include "docket_def.h"

int docket_accept(Docket *docket, int fd) {
  LOGD("fd = %d", fd);
  struct sockaddr_storage remote_address;
  socklen_t remote_address_len = sizeof(remote_address);
  int remote_fd = accept(fd, (struct sockaddr *) &remote_address, &remote_address_len);
  if (remote_fd == -1) {
    if (errno != EAGAIN) {
      LOGD("accept failed: %s", devent_errno());
    }
    return -1;
  }
  LOGD("accept fd = %d", remote_fd);

  if (devent_turn_on_flags(remote_fd, O_NONBLOCK)) {
    LOGD("turn_on_flags failed: %s", errno, devent_errno());
    return -1;
  }

  // update event to read write mod
  DocketEvent *event = DocketEvent_new(docket, remote_fd, NULL);
  event->ev = DEVENT_READ_WRITE;

  devent_update_events(docket->fd, remote_fd, event->ev, 0);

  // create event
  event->connected = true;
  Docket_add_event(event);

  // listener callback
  DocketListener *listener = Docket_get_listener(docket, fd);
  if (listener) {
#ifdef DEVENT_SSL
    if (listener->ssl_ctx) {
      event->ssl_handshaking = true;
      event->ssl = SSL_new(docket->ssl_ctx);
      SSL_set_fd(event->ssl, event->fd);
      SSL_set_accept_state(event->ssl);

      // delay connect callback after ssl handshake
      event->listener = listener;
    } else
#endif
    {
      listener->cb(listener, remote_fd, (struct sockaddr *) &remote_address, remote_address_len, event, listener->ctx);
    }
  }

  return 0;
}
#endif
