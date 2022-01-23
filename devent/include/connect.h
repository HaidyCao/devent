//
// Created by Haidy on 2021/11/13.
//

#ifndef DOCKET_CONNECT_H
#define DOCKET_CONNECT_H

#include <stdbool.h>

#include "event.h"

/**
 * connect to hostname
 * @param docket docket
 * @param fd -1 or fd
 * @param host domain or ip address of ipv4 or ipv6
 * @param port port
 * @param ssl  use ssl
 * @return event
 */
DocketEvent *DocketEvent_connect(Docket *docket, int fd, struct sockaddr *address, socklen_t socklen);

/**
 * crate a event from fd
 * @param docket    docket
 * @param fd        fd
 * @return event
 */
DocketEvent *DocketEvent_create(Docket *docket, int fd);

/**
 * connect remote by hostname and port
 * @param docket    docket
 * @param fd        fd
 * @param host      remote host
 * @param port      remote port
 * @return event
 */
DocketEvent *DocketEvent_connect_hostname(Docket *docket, int fd, const char *host, unsigned short port);

#ifdef DEVENT_SSL

/**
 * connect to remote address use ssl
 * @param docket docket
 * @param address address
 * @param socklen address len
 * @return event
 */
DocketEvent *DocketEvent_connect_ssl(Docket *docket, int fd, struct sockaddr *address, socklen_t socklen);

/**
 * connect remote by hostname and port
 * @param docket    docket
 * @param fd        fd
 * @param host      remote host
 * @param port      remote port
 * @return event
 */
DocketEvent *DocketEvent_connect_hostname_ssl(Docket *docket, int fd, const char *host, unsigned short port);

#endif

#endif //DOCKET_CONNECT_H
