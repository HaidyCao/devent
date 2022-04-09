#include "c_array_list.h"
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
  DocketEvent *event;

  WSABUF buf;
} FileIoContext;

static FileIoContext *FileIoContext_new(DocketEvent *event) {
  FileIoContext *io = calloc(1, sizeof(FileIoContext));
  io->event = event;
  io->buf.buf = malloc(DOCKET_RECV_BUF_SIZE);
  io->buf.len = DOCKET_RECV_BUF_SIZE;

  return io;
}

static void io_completion_routine(
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransfered,
    LPOVERLAPPED lpOverlapped
);

static void win_read_file(FileIoContext *io) {
  DocketEvent *event = io->event;
  if (!ReadFileEx((HANDLE) event->fd, io->buf.buf, io->buf.len, (LPOVERLAPPED) io, io_completion_routine)) {
    DWORD error = GetLastError();
    if (error != ERROR_IO_PENDING) {
      if (event->event_cb) {
        event->event_cb(event, DEVENT_READ | DEVENT_ERROR, event->ctx);
      }
      return;
    }
  }
}

static void io_completion_routine(
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransfered,
    LPOVERLAPPED lpOverlapped
) {
  FileIoContext *io = (FileIoContext *) lpOverlapped;
  if (dwErrorCode != ERROR_SUCCESS) {
    LOGE("%s", devent_errno());
    return;
  }
  DocketBuffer *in_buffer = DocketEvent_get_in_buffer(io->event);
  DocketBuffer_write(in_buffer, io->buf.buf, dwNumberOfBytesTransfered);

  if (io->event->read_cb) {
    io->event->read_cb(io->event, io->event->ctx);
  }

  win_read_file(io);
}

#else
#include <unistd.h>
#include <sys/fcntl.h>
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

static DWORD WINAPI docket_win_std_in_thread(LPVOID lpThreadParameter) {
  DocketEvent *event = lpThreadParameter;

  while (true) {
    char buffer[1024 * 10];
    bzero(buffer, sizeof(buffer));
    fgets(buffer, sizeof(buffer), stdin);

    DocketBuffer *in_buffer = DocketEvent_get_in_buffer(event);
    DocketBuffer_write(in_buffer, buffer, strlen(buffer));

    if (event->read_cb) {
      event->read_cb(event, event->ctx);
    } else {
      LOGE("stdin need set read callback");
      break;
    }
  }
  return -1;
}

#else
// TODO
#endif

DocketEvent *Docket_create_stdin_event(Docket *docket) {
#ifdef WIN32
  // TODO new thread
  HANDLE file = CreateFile("CONIN$", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
  if (file == INVALID_HANDLE_VALUE) {
    return NULL;
  }

  HANDLE hThread;
  DWORD threadId;

  DocketEvent *event = DocketEvent_new(docket, (SOCKET) file, NULL);
  event->is_file = true;
  event->ev = DEVENT_READ;
  // prepare for read
//  DocketEvent_readFile(event, NULL);
  hThread = CreateThread(NULL, 0, docket_win_std_in_thread, event, 0, &threadId);
  if (hThread == NULL) {
    DocketEvent_free(event);
    return NULL;
  }

  Docket_add_event(event);
  return Docket_find_event(docket, (SOCKET) file);
#else
  DocketEvent *event = DocketEvent_new(docket, STDIN_FILENO, NULL);
  devent_turn_on_flags(STDIN_FILENO, O_NONBLOCK);
  devent_update_events(docket->fd, event->fd, DEVENT_READ, DEVENT_MOD_ADD);
  Docket_add_event(event);
  return event;
#endif
}

#ifdef WIN32

#else
DocketEvent *Docket_create_file_event(Docket *docket, int fd) {
  DocketEvent *event = DocketEvent_new(docket, fd, NULL);
  devent_turn_on_flags(fd, O_NONBLOCK);
  devent_update_events(docket->fd, event->fd, DEVENT_READ, DEVENT_MOD_ADD);
  Docket_add_event(event);
  return event;
}
#endif