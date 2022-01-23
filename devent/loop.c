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
#endif

int Docket_loop(Docket *docket) {
#ifdef __APPLE__
  return kqueue_loop(docket);
#elif __linux__ || __ANDROID__
  epoll_loop(docket);
#else
  return -1;
#endif
}
