//
// Created by haidy on 2022/3/21.
//

#include "file_event.h"
#include "docket.h"

DocketEvent *DocketEvent_create_stdin_event(Docket *docket) {
#ifdef WIN32
  HANDLE file = CreateFile("CONIN$", FILE_GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
  if (file == NULL) {
    return NULL;
  }
  CreateIoCompletionPort(file, docket->fd, (ULONG_PTR) file, 0);
  DocketEvent *event = DocketEvent_new(docket, (SOCKET) file, NULL);
  event->is_file = true;
  Docket_add_event(event);
  // TODO prepare for read

  return event;
#else
  // TODO unix
#endif
}