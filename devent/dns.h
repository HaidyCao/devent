//
// Created by Haidy on 2021/9/25.
//

#ifndef DOCKET_DNS_H
#define DOCKET_DNS_H

#include <stdbool.h>

#ifdef WIN32
#include <WinSock2.h>
#endif
#include "def.h"
#include "docket_def.h"

#ifdef DEVENT_SSL
#include "openssl/ssl.h"
#endif

/**
 * dns event
 */
struct docket_dns_event {
  Docket *docket;
  /**
   * dns fd
   */
  SOCKET dns_fd;
  /**
   * domain
   */
  char *domain;
  DocketEvent *event;

  unsigned short port;

#ifdef DEVENT_SSL
  bool event_ssl;
#endif
};

typedef struct docket_dns_context DocketDnsContext;

struct docket_dns_context {
  DocketEvent *event;
  char *domain;
  unsigned short port;
#ifdef DEVENT_SSL
  bool event_ssl;
#endif
};

DocketDnsContext *DocketDnsContext_new(DocketEvent *event, char *domain, unsigned short port);

void DocketDnsContext_free(DocketDnsContext *dns_context);

DocketDnsEvent *DocketDnsEvent_new(Docket *docket, SOCKET fd, const char *domain, unsigned short port);

/**
 * free dns event
 * @param event
 */
void DocketDnsEvent_free(DocketDnsEvent *event);

/**
 * write dns packet to dns server
 * @param event dns udp event
 */
void docket_dns_write(DocketEvent *event);

/**
 * handle dns write event
 * @param event dns event
 */
void devent_dns_write(DocketDnsEvent *event);

/**
 * handle dns read event
 * @param event dns event
 */
void devent_dns_read(DocketDnsEvent *event);

/**
 * read dns data from dns server
 * @param dns_event dns event
 */
void docket_dns_read(DocketEvent *dns_event);

#endif //DOCKET_DNS_H
