//
// Created by haidy on 2022/3/21.
//

#include "file_event_internal.h"
#include "docket.h"
#include "log.h"
#include "utils_internal.h"

#ifdef WIN32
typedef struct {
  OVERLAPPED lpOverlapped;
  int op;
  DocketEvent *event;

  WSABUF buf;
} FileIoContext;

static FileIoContext *FileIoContext_new(DocketEvent *event) {
  FileIoContext *io = calloc(1, sizeof(FileIoContext));
  io->event = event;

  return io;
}

static void io_completion_routine(
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransfered,
    LPOVERLAPPED lpOverlapped
) {
  IO_CONTEXT *io = lpOverlapped;
  if (dwErrorCode) {
//    DocketEvent *event = Docket_find_event(lpOverlapped.do);

    return;
  }
}

static void win_read_file(FileIoContext *io) {
  DocketEvent *event = io->event;
  if (!ReadFile((HANDLE) event->fd, io->buf.buf, io->buf.len, NULL, (LPOVERLAPPED) io)) {
    DWORD error = GetLastError();
    if (error != ERROR_IO_PENDING) {
      if (event->event_cb) {
        event->event_cb(event, DEVENT_READ | DEVENT_ERROR, event->ctx);
      }
      return;
    }
  }
}
#else
#include <unistd.h>
#endif

#ifdef WIN32
void DocketEvent_readFile(DocketEvent *event, IO_CONTEXT *io) {
  if (!devent_read_enable(event)) {
    return;
  }

  if (io == NULL) {
    io = IO_CONTEXT_new(IOCP_OP_READ, event->fd);
  }

  if (!ReadFile((HANDLE) event->fd, io->buf.buf, io->buf.len, NULL, (LPOVERLAPPED) io)) {
    DWORD error = GetLastError();
    if (error != ERROR_IO_PENDING) {
      if (event->event_cb) {
        event->event_cb(event, DEVENT_READ | DEVENT_ERROR, event->ctx);
      }
      return;
    }
  }
}
#else
// TODO
#endif

DocketEvent *DocketEvent_create_stdin_event(Docket *docket) {
#ifdef WIN32
  // TODO new thread
  HANDLE file = CreateFile("CONIN$", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
  if (file == INVALID_HANDLE_VALUE) {
    return NULL;
  }
//  BindIoCompletionCallback(file, io_completion_routine, 0);
//  FileIoContext *io = FileIoContext_new(file);
//  io->op = IOCP_OP_READ;
//  win_read_file(io);

  CreateIoCompletionPort(file, docket->fd, (ULONG_PTR) file, 0);
  DocketEvent *event = DocketEvent_new(docket, (SOCKET) file, NULL);
  event->is_file = true;
  event->ev = DEVENT_READ;
  Docket_add_event(event);
  // prepare for read
  DocketEvent_readFile(event, NULL);

  return Docket_find_event(docket, (SOCKET) file);
#else
  DocketEvent *event = DocketEvent_new(docket, STDIN_FILENO, NULL);
  Docket_add_event(event);
  devent_update_events(docket->fd, event->fd, DEVENT_READ, DEVENT_MOD_ADD);
  return event;
#endif
}