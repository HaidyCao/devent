//
// Created by Haidy on 2021/11/13.
//

#ifndef DOCKET_CONNECT_H
#define DOCKET_CONNECT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#ifdef WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#endif

#include "event.h"

/**
 * connect to address
 * @param docket docket
 * @param fd -1 or fd
 * @param address address
 * @param socklen address len
 * @return event
 */
DocketEvent *DocketEvent_connect(Docket *docket, SOCKET fd, struct sockaddr *address, socklen_t socklen);

/**
 * connect to hostname
 * @param docket docket
 * @param address address
 * @param socklen address len
 * @param fd -1 or fd
 * @return event
 */
DocketEvent *DocketEvent_create_udp(Docket *docket, SOCKET fd, struct sockaddr *address, socklen_t socklen);

/**
 * crate a event from fd
 * @param docket    docket
 * @param fd        fd
 * @return event
 */
DocketEvent *DocketEvent_create(Docket *docket, SOCKET fd);

/**
 * connect remote by hostname and port
 * @param docket    docket
 * @param fd        fd
 * @param host      remote host
 * @param port      remote port
 * @return event
 */
DocketEvent *DocketEvent_connect_hostname(Docket *docket, SOCKET fd, const char *host, unsigned short port);

#ifdef DEVENT_SSL

/**
 * connect to remote address use ssl
 * @param docket docket
 * @param address address
 * @param socklen address len
 * @return event
 */
DocketEvent *DocketEvent_connect_ssl(Docket *docket, SOCKET fd, struct sockaddr *address, socklen_t socklen);

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

#ifdef __cplusplus
}
#endif

#endif //DOCKET_CONNECT_H
