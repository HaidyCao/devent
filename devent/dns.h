//
// Created by Haidy on 2021/9/25.
//

#ifndef DOCKET_DNS_H
#define DOCKET_DNS_H

#include <stdbool.h>

#include "def.h"
#include "docket_def.h"

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

/**
 * write dns packet to dns server
 * @param event dns udp event
 */
void docket_dns_write(DocketEvent *event);

/**
 * read dns data from dns server
 * @param dns_event dns event
 */
void docket_dns_read(DocketEvent *dns_event);

#endif //DOCKET_DNS_H
