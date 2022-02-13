//
// Created by Haidy on 2021/11/20.
//

#ifndef DOCKET_DOCKET_DEF_H
#define DOCKET_DOCKET_DEF_H

#ifndef WIN32
#include <sys/socket.h>
typedef int SOCKET;
#else
#define IOCP_OP_ACCEPT 0
#define IOCP_OP_READ 1
#define IOCP_OP_WRITE 2
#define IOCP_OP_CONNECT 4

#define IOCP_OP_READ_WRITE (IOCP_OP_READ | IOCP_OP_WRITE)

typedef struct {
  OVERLAPPED lpOverlapped;
  int op;
  SOCKET socket;

  char lpOutputBuffer[256];
  WSABUF recvBuf;
  DWORD lpFlags;
  struct sockaddr *local;
  int local_len;
  struct sockaddr *remote;
  int remote_len;

} IO_CONTEXT;

void IO_CONTEXT_free(IO_CONTEXT *ctx);

#endif

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
#ifdef WIN32
  HANDLE fd;
#else
  int fd;
#endif

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
