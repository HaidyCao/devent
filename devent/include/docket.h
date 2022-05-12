//
// Created by Haidy on 2021/9/21.
//

#ifndef DOCKET_DOCKET_H
#define DOCKET_DOCKET_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <sys/socket.h>
#endif
#include "def.h"

/**
 * create a new Docket struct
 */
Docket *Docket_new();

/**
 * Docket add listener
 * @param docket docket
 * @param listener listener
 */
void Docket_add_listener(Docket *docket, DocketListener *listener);

/**
 * docket set dns server
 * @param docket docket
 * @param server server
 * @return -1 failed
 */
int Docket_set_dns_server(Docket *docket, char *server);

/**
 * set dns server callback
 * @param docket docket
 * @param fn function
 */
void Docket_set_dns_server_callback(Docket *docket, docket_fn_custom_dns_server fn);

/**
 * get listener of fd
 * @param docket docket
 * @param fd fd
 * @return listener
 */
DocketListener *Docket_get_listener(Docket *docket, SOCKET fd);

/**
 * find event from docket
 * @param docket docket
 * @param fd event fd
 * @return event or NULL
 */
DocketEvent *Docket_find_event(Docket *docket, SOCKET fd);

/**
 * add event to docket
 * @param event event
 */
void Docket_add_event(DocketEvent *event);

/**
 * remove event from docket
 * @param event event
 */
void Docket_remove_event(DocketEvent *event);

/**
 * Docket will loop if no error
 * @return -1 loop finished
 */
void Docket_loop(Docket *docket);

void Docket_set_log_level(int level);

#ifdef __cplusplus
}
#endif

#endif //DOCKET_DOCKET_H
