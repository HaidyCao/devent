//
// Created by Haidy on 2021/11/20.
//

#ifndef DOCKET_DOCKET_DEF_H
#define DOCKET_DOCKET_DEF_H

#ifndef WIN32
#include <sys/socket.h>
#else
#include <WS2tcpip.h>

#define IOCP_OP_ACCEPT 0
#define IOCP_OP_READ 1
#define IOCP_OP_WRITE 2
#define IOCP_OP_CONNECT 4

#ifndef DOCKET_RECV_BUF_SIZE
#define DOCKET_RECV_BUF_SIZE 40960
#endif

typedef struct {
  OVERLAPPED lpOverlapped;
  int op;
  SOCKET socket;

  char *lpOutputBuffer;
  WSABUF buf;
  DWORD lpFlags;
  struct sockaddr *local;
  int local_len;
  struct sockaddr *remote;
  int remote_len;

} IO_CONTEXT;

IO_CONTEXT *IO_CONTEXT_new(int op, SOCKET socket);
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
