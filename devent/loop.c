//
// Created by Haidy on 2021/11/20.
//

#ifdef __APPLE__

#include <sys/event.h>
#include <unistd.h>

#elif __linux__ || __ANDROID__

#include <sys/epoll.h>

#endif

#include "def.h"
#include "event.h"
#include "docket_def.h"
#include "c_sparse_array.h"
#include "accept.h"
#include "read.h"
#include "write.h"
#include "event_def.h"
#include "log.h"
#include "dns.h"
#include "utils.h"
#include "buffer_def.h"
#include "win_def.h"
#include "listener_def.h"
#include "file_event_internal.h"

#define MAX_EVENT 16

#ifdef __APPLE__

static int kqueue_loop(Docket *docket) {
  struct kevent events[MAX_EVENT];
  while (true) {
    if (docket->fd == -1) {
      return -1;
    }
    int n = kevent(docket->fd, NULL, 0, events, MAX_EVENT, NULL);
    if (n == -1) {
      continue;
    }

    for (int i = 0; i < n; ++i) {
      if (docket->fd == -1) {
        return -1;
      }

      int filter = events[i].filter;
      if (filter == EVFILT_TIMER) {
        // event is timer

        continue;
      }
      int efd = (int) (intptr_t) events[i].udata;

      if (filter == EVFILT_READ) {
        DocketListener *listener = CSparseArray_get(docket->listeners, efd);
        if (listener != NULL) {
          // accept socket
          docket_accept(docket, efd);
          continue;
        }
        DocketDnsEvent *dns = CSparseArray_get(docket->dns_events, efd);
        if (dns != NULL) {
          devent_dns_read(dns);
          continue;
        }
        // TODO event read
        DocketEvent *event = Docket_find_event(docket, efd);
        if (event) {
          docket_on_event_read(event);
        }
      } else if (filter == EVFILT_WRITE) {
        DocketDnsEvent *dns = CSparseArray_get(docket->dns_events, efd);
        if (dns != NULL) {
          devent_dns_write(dns);
          continue;
        }

        DocketEvent *event = Docket_find_event(docket, efd);
        if (event == NULL) {
          LOGD("event is NULL: fd = %d", efd);
          close(efd);
          continue;
        }

        if (!event->connected) {
          event->connected = true;

          if (event->event_cb) {
            event->event_cb(event, DEVENT_CONNECT, event->ctx);

            event = Docket_find_event(docket, efd);
            if (event == NULL) {
              continue;
            }
          }

          // TODO timeout
        }

        docket_on_event_write(event);
      } else {
        // unknown filter
      }
    }
  }
}

#elif __linux__ || __ANDROID__
int epoll_loop(Docket *docket) {
    struct epoll_event events[MAX_EVENT];
    bzero(events, sizeof(events));
    while (true) {
        if (docket->fd == -1) {
            return -1;
        }
        int n = epoll_wait(docket->fd, events, MAX_EVENT, -1);

        for (int i = 0; i < n; ++i) {
            uint32_t ev = events[i].events;
            int event_fd = events[i].data.fd;

            if ((ev & EPOLLERR) || (ev & EPOLLHUP)) {
                // epoll error
                continue;
            }


        }
    }
}

#elif WIN32
#include <WS2tcpip.h>
#include <MSWSock.h>

static bool prepare_read(IO_CONTEXT *io, DocketEvent *event) {
  if (devent_read_enable(event)) {
    io->op = IOCP_OP_READ;
    io->lpFlags = 0;

    if (WSARecv(io->socket, &io->buf, 1,
                NULL, &io->lpFlags, (LPWSAOVERLAPPED) io, NULL) == SOCKET_ERROR
        && WSAGetLastError() != WSA_IO_PENDING) {
      LOGI("WSARecv failed: %d", WSAGetLastError());
      closesocket(io->socket);
      DocketEvent_free(event);
      IO_CONTEXT_free(io);
      return false;
    }
  } else {
    IO_CONTEXT_free(io);
  }
  return true;
}

const char local_address[INET6_ADDRSTRLEN];
const char remote_address[INET6_ADDRSTRLEN];

static void iocp_loop(Docket *docket) {
  DWORD num;
  SOCKET client;
  IO_CONTEXT *io = NULL;

  GUID GuidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
  LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockaddrs = NULL;

  while (true) {
    if (GetQueuedCompletionStatus(docket->fd, &num, (PULONG_PTR) &client, (LPOVERLAPPED *) &io, WSA_INFINITE)) {
      if (io && client) {
        switch (io->op) {
          case IOCP_OP_ACCEPT: {
            if (lpfnGetAcceptExSockaddrs == NULL) {
              DWORD dwBytes;
              int r = WSAIoctl(io->socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
                               &GuidGetAcceptExSockaddrs, sizeof(GuidGetAcceptExSockaddrs),
                               &lpfnGetAcceptExSockaddrs, sizeof(lpfnGetAcceptExSockaddrs),
                               &dwBytes, NULL, NULL);
              if (r == SOCKET_ERROR) {
                LOGD("accept failed: %s", devent_errno());
                continue;
              }
            }

            lpfnGetAcceptExSockaddrs(io->lpOutputBuffer, 0, io->local_len + 16, io->remote_len + 16,
                                     &io->local, &io->local_len, &io->remote, &io->remote_len);
            LOGD("accept socket local: %s:%d, remote: %s:%d",
                 inet_ntop(io->local->sa_family,
                           &((struct sockaddr_in *) io->local)->sin_addr,
                           (char *) local_address,
                           INET6_ADDRSTRLEN),
                 htons(((struct sockaddr_in *) io->local)->sin_port),
                 inet_ntop(io->remote->sa_family,
                           &((struct sockaddr_in *) io->remote)->sin_addr,
                           (char *) remote_address,
                           INET6_ADDRSTRLEN),
                 htons(((struct sockaddr_in *) io->remote)->sin_port));

            SOCKET
                acceptSocket = WSASocketW(io->local->sa_family, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

            IO_CONTEXT *new_io = IO_CONTEXT_new(IOCP_OP_ACCEPT, acceptSocket);
            new_io->local_len = io->local_len;
            new_io->remote_len = io->remote_len;

            docket_accept(docket, client, acceptSocket, new_io);

            // handle client socket
            CreateIoCompletionPort((HANDLE) io->socket, docket->fd, io->socket, 0);

            DocketListener *listener = Docket_get_listener(docket, client);
            DocketEvent *event = Docket_find_event(docket, io->socket);
            if (listener && listener->cb && event) {
              listener->cb(listener, io->socket, io->remote, io->remote_len, event, listener->ctx);
              // read and send data

              if (prepare_read(IO_CONTEXT_new(IOCP_OP_READ, io->socket), event)) {
                docket_on_event_write(event);
              }
            }

            IO_CONTEXT_free(io);
            break;
          }
          case IOCP_OP_READ: {
            DocketEvent *event = Docket_find_event(docket, io->socket);
            if (event) {
              if (num == 0) {
                if (event->event_cb) {
                  event->event_cb(event, DEVENT_READ_EOF, event->ctx);
                  event = Docket_find_event(docket, io->socket);
                }
                if (event) {
                  DocketEvent_free(event);
                }
                IO_CONTEXT_free(io);
                continue;
              }

              docket_on_handle_read_data(io, num, event);
              event = Docket_find_event(docket, io->socket);
              if (event) {
                if (event->is_file) {
                  DocketEvent_readFile(event, io);
                  continue;
                }
                prepare_read(io, event);
              }
            } else {
              LOGE("find event failed: %d", io->socket);
              closesocket(io->socket);
              IO_CONTEXT_free(io);
            }
            break;
          }
          case IOCP_OP_WRITE: {
            SOCKET fd = io->socket;
            DocketEvent *event = Docket_find_event(docket, fd);
            if (event) {
              if (event->write_cb) {
                event->write_cb(event, event->ctx);
                if (event != Docket_find_event(docket, fd)) {
                  return;
                }
              }

              if (io->buf.len < num) {
                // need write left data
                ULONG buf_len = io->buf.len;
                char *data = io->buf.buf;

                io->buf.len = buf_len - num;
                io->buf.buf = malloc(io->buf.len);
                memcpy(io->buf.buf, data + num, io->buf.len);

                // send to remote
                if (DocketBuffer_win_send(io,
                                          WSA_FLAG_OVERLAPPED,
                                          event->remote_address,
                                          event->remote_address_len) != 0) {
                  LOGE("DocketBuffer_send failed: %s", devent_errno());
                  IO_CONTEXT_free(io);
                  DocketEvent_free(event);
                  return;
                }
                return;
              }

              IO_CONTEXT_free(io);
              docket_on_event_write(event);
            }
            break;
          }
          case IOCP_OP_CONNECT: {
            IO_CONTEXT_free(io);
            DocketEvent *event = Docket_find_event(docket, client);

            // read and send data
            if (prepare_read(IO_CONTEXT_new(IOCP_OP_READ, client), event)) {
              docket_on_event_write(event);
            }

            if (event) {
              event->connected = true;
              if (event->event_cb) {
                event->event_cb(event, DEVENT_CONNECT, event->ctx);

                event = Docket_find_event(docket, client);
                if (event == NULL) {
                  continue;
                }
              }
            } else {
              LOGE("find event failed: %d", io->socket);
              closesocket(io->socket);
              IO_CONTEXT_free(io);
            }
            break;
          }
          default:
            // TODO
            break;
        }
      } else {
        // TODO
      }
    } else {
      LOGD("GetQueuedCompletionStatus error");
    }
  }
}

#endif

void Docket_loop(Docket *docket) {
#ifdef __APPLE__
  kqueue_loop(docket);
#elif __linux__ || __ANDROID__
  epoll_loop(docket);
#elif WIN32
  iocp_loop(docket);
#else
#endif
}
