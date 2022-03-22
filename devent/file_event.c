//
// Created by haidy on 2022/3/21.
//

#include "file_event_internal.h"
#include "docket.h"

void DocketEvent_readFile(DocketEvent *event, IO_CONTEXT *io) {
  if (io == NULL) {
    io = IO_CONTEXT_new(IOCP_OP_READ, event->fd);
  }

#ifdef WIN32
  if (!ReadFile((HANDLE) event->fd, io->buf.buf, io->buf.len, NULL, (LPOVERLAPPED) io)) {
    DWORD error = GetLastError();
    if (error != ERROR_IO_PENDING) {
      if (event->event_cb) {
        event->event_cb(event, DEVENT_READ | DEVENT_ERROR, event->ctx);
      }
      return;
    }
  }
#else
  // TODO
#endif
}

DocketEvent *DocketEvent_create_stdin_event(Docket *docket) {
#ifdef WIN32
  HANDLE file = CreateFile("CONIN$", FILE_GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
  if (file == NULL) {
    return NULL;
  }
  CreateIoCompletionPort(file, docket->fd, (ULONG_PTR) file, 0);
  DocketEvent *event = DocketEvent_new(docket, (SOCKET) file, NULL);
  event->is_file = true;
  event->ev = DEVENT_READ;
  Docket_add_event(event);
  // prepare for read
  DocketEvent_readFile(event, NULL);

  return Docket_find_event(docket, (SOCKET) file);
#else
  // TODO unix
#endif
}