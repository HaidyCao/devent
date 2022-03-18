#include <WinSock2.h>
#include <stdio.h>
#include <errno.h>
#include <MSWSock.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define IOCP_OP_ACCEPT 0
#define IOCP_OP_READ 1
#define IOCP_OP_WRITE 2
#define IOCP_OP_CONNECT 4

typedef struct _io_context {
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

static DWORD dwBytes;
static GUID GuidAcceptEx = WSAID_ACCEPTEX;
static LPFN_ACCEPTEX lpfnAcceptEx = NULL;

static GUID GuidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
static LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockaddrs = NULL;

static GUID GuidConnectex = WSAID_CONNECTEX;
static LPFN_CONNECTEX lpfnConnectex = NULL;

static void callAccept(SOCKET listener, socklen_t socklen) {
  SOCKET acceptSocket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

  IO_CONTEXT *io = calloc(1, sizeof(IO_CONTEXT));
  if (io == NULL) {
    exit(-1);
  }
  io->op = IOCP_OP_ACCEPT;
  io->socket = acceptSocket;
  DWORD addressLength = socklen + 16;
  io->local_len = sizeof(struct sockaddr_storage);
  io->remote_len = sizeof(struct sockaddr_storage);
  io->recvBuf.len = 5;
  io->recvBuf.buf = malloc(io->recvBuf.len);

  BOOL r = lpfnAcceptEx(listener, acceptSocket, io->lpOutputBuffer, 0, addressLength, addressLength, &dwBytes,
                        (LPOVERLAPPED) io);
  if (r == FALSE && WSAGetLastError() != WSA_IO_PENDING) {
    wprintf(L"AcceptEx failed with error: %u\n", WSAGetLastError());
    closesocket(listener);
    closesocket(acceptSocket);
    WSACleanup();
    exit(-1);
  }
}

static void callRead(IO_CONTEXT *io) {
  io->op = IOCP_OP_READ;
  io->lpFlags = 0;
  if (WSARecv(io->socket, &io->recvBuf, 1,
              NULL, &io->lpFlags, io, NULL) == SOCKET_ERROR
      && WSAGetLastError() != WSA_IO_PENDING) {
    printf("WSARecv failed: %d\n", WSAGetLastError());
    closesocket(io->socket);
    WSACleanup();
    exit(-1);
  }
}

static void callConnect(HANDLE cp) {
  SOCKET fd = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
  if (fd == INVALID_SOCKET) {
    WSACleanup();
    exit(-1);
  }

  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  const char *ip = "110.242.68.3";
  inet_pton(AF_INET, ip, &address.sin_addr);
  address.sin_family = AF_INET;
  address.sin_port = htons(80);

  IO_CONTEXT *io = calloc(1, sizeof(IO_CONTEXT));
  if (io == NULL) {
    exit(-1);
  }
  io->op = IOCP_OP_CONNECT;
  io->socket = fd;
  io->remote = malloc(sizeof(address));
  memcpy(io->remote, &address, sizeof(address));
  io->remote_len = sizeof(address);
  io->recvBuf.len = 5;
  io->recvBuf.buf = malloc(io->recvBuf.len);

  io->local = malloc(sizeof(struct sockaddr_in));
  io->local_len = io->remote_len;

  struct sockaddr_in *local = io->local;
  local->sin_family = AF_INET;
  local->sin_addr.s_addr = htonl(INADDR_ANY);
  local->sin_port = htons(0);

//	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, io->local, io->local_len);
  if (bind(fd, io->local, io->local_len) == SOCKET_ERROR) {
    printf("bind failed: %d\n", WSAGetLastError());
    closesocket(io->socket);
    WSACleanup();
    exit(-1);
  }

  CreateIoCompletionPort((HANDLE) fd, cp, fd, 0);

  if (!lpfnConnectex(fd, (const struct sockaddr *) &address, sizeof(address), NULL, 0, NULL, io)
      && WSAGetLastError() != WSA_IO_PENDING) {
    printf("Connectex failed: %d\n", WSAGetLastError());
    closesocket(io->socket);
    WSACleanup();
    exit(-1);
  }

  getsockname(fd, io->local, &io->local_len);
}

#include "echo_server_test.h"
#include "telnet_test.h"
#include "ssl_telnet_test.h"

int main(int argc, char **args) {
  if (TRUE) {
    ssl_telnet_start();
//    telnet_start();
//    echo_server_start();
    return -1;
  }

  WSADATA lpWsaData;
  int startUpResult = WSAStartup(WINSOCK_VERSION, &lpWsaData);
  if (startUpResult != 0) {
    int l = WSAGetLastError();
    return -1;
  }
  HANDLE cp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

  if (cp == NULL) {
    DWORD dw = GetLastError();

    return -1;
  }

  SOCKET fd = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_family = AF_INET;
  address.sin_port = htons(1188);

  // reuse address
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (struct sockaddr *) &address, sizeof(address));

  int r = bind(fd, (struct sockaddr *) &address, sizeof(address));
  if (r == -1) {
    printf("errno: %d, %s", WSAGetLastError(), strerror(errno));
    return -1;
  }
  r = listen(fd, 20);
  CreateIoCompletionPort((HANDLE) fd, cp, (ULONG_PTR) fd, 0);

  {
    r = WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER,
                 &GuidAcceptEx, sizeof(GuidAcceptEx),
                 &lpfnAcceptEx, sizeof(lpfnAcceptEx),
                 &dwBytes, NULL, NULL);
    if (r == SOCKET_ERROR) {
      wprintf(L"WSAIoctl failed with error: %u\n", WSAGetLastError());
      closesocket(fd);
      WSACleanup();
      return 1;
    }

    r = WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER,
                 &GuidGetAcceptExSockaddrs, sizeof(GuidGetAcceptExSockaddrs),
                 &lpfnGetAcceptExSockaddrs, sizeof(lpfnGetAcceptExSockaddrs),
                 &dwBytes, NULL, NULL);
    if (r == SOCKET_ERROR) {
      wprintf(L"WSAIoctl failed with error: %u\n", WSAGetLastError());
      closesocket(fd);
      WSACleanup();
      return 1;
    }

    r = WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER,
                 &GuidConnectex, sizeof(GuidConnectex),
                 &lpfnConnectex, sizeof(lpfnConnectex),
                 &dwBytes, NULL, NULL);
    if (r == SOCKET_ERROR) {
      wprintf(L"WSAIoctl failed with error: %u\n", WSAGetLastError());
      closesocket(fd);
      WSACleanup();
      return 1;
    }

    callAccept(fd, sizeof(address));
  }

  while (TRUE) {
    DWORD num = 0;
    //SOCKET client;
    ULONG_PTR key = 0;
    IO_CONTEXT *io = NULL;
    //LPOVERLAPPED io = NULL;

    BOOL s = GetQueuedCompletionStatus(cp, &num, &key, &io, WSA_INFINITE);
    if (s) {
      printf("success: %d\n", s);
      //free(io);
      if (io->op == IOCP_OP_ACCEPT) {
        callAccept(fd, sizeof(address));
        lpfnGetAcceptExSockaddrs(io->lpOutputBuffer, 0, sizeof(address) + 16, sizeof(address) + 16,
                                 &io->local, &io->local_len, &io->remote, &io->remote_len);//*/

        //r = getpeername(io->socket, &io->remote, &io->remote_len);
        printf("local: %s:%d, remote: %s:%d\n",
               inet_ntoa(((struct sockaddr_in *) io->local)->sin_addr),
               htons(((struct sockaddr_in *) io->local)->sin_port),
               inet_ntoa(((struct sockaddr_in *) io->remote)->sin_addr),
               htons(((struct sockaddr_in *) io->remote)->sin_port));

        // ready read data
        CreateIoCompletionPort(io->socket, cp, io->socket, 0);
        //callRead(io);
        callConnect(cp);
      } else if (io->op == IOCP_OP_READ) {
        printf("read data length: %d\n", num);

        callRead(io);
      } else if (io->op == IOCP_OP_CONNECT) {
        printf("on connect: %d\n", io->socket);

        callRead(io);
      } else {
        //
        printf("bad op: %d", io->op);
      }

    } else {
      printf("failed: %d", s);
    }
  }
  return 0;
}