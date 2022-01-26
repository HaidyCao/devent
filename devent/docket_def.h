//
// Created by Haidy on 2021/11/20.
//

#ifndef DOCKET_DOCKET_DEF_H
#define DOCKET_DOCKET_DEF_H

#include <sys/socket.h>

#include "c_sparse_array.h"
#include "c_linked_list.h"

#ifdef DEVENT_SSL
#include "openssl/ssl.h"
#include "openssl/err.h"

#endif

typedef struct sockaddr *SOCK_ADDRESS;

typedef struct dns_server {
    SOCK_ADDRESS address;
    socklen_t socklen;
} DnsServer;

struct docket {
    /**
     * listened fd
     */
    int fd;

    /**
     * dns server address
     */
    DnsServer dns_server;

    /**
     * listeners
     */
    CSparseArray *listeners;

    /**
     * dns events
     */
    CSparseArray *dns_events;

    /**
     * events
     */
    CSparseArray *events;

#ifdef DEVENT_SSL
    SSL_CTX *ssl_ctx;
#endif
};

#endif //DOCKET_DOCKET_DEF_H
