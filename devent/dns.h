//
// Created by Haidy on 2021/9/25.
//

#ifndef DOCKET_DNS_H
#define DOCKET_DNS_H

#include <stdbool.h>

#include "def.h"

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
  int dns_fd;
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

DocketDnsEvent *DocketDnsEvent_new(Docket *docket, int fd, const char *domain, unsigned short port);

/**
 * free dns event
 * @param event
 */
void DocketDnsEvent_free(DocketDnsEvent *event);

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

#endif //DOCKET_DNS_H
