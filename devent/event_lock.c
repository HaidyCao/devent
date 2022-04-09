//
// Created by haidy on 2022/4/9.
//
#ifdef DEVENT_MULTI_THREAD
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "event_lock.h"

#ifdef WIN32
#include <combaseapi.h>

#define GUID_LEN 64
static char *generate(char buf[GUID_LEN]) {
  GUID guid;

  if (CoCreateGuid(&guid)) {
    return "";
  }

  sprintf(buf,
          "%08lX-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X",
          guid.Data1, guid.Data2, guid.Data3,
          guid.Data4[0], guid.Data4[1], guid.Data4[2],
          guid.Data4[3], guid.Data4[4], guid.Data4[5],
          guid.Data4[6], guid.Data4[7]);

  return buf;
}
#endif

DEventLock *DEventLock_new() {
  DEventLock *lock = calloc(1, sizeof(DEventLock));
#ifdef WIN32
  char lpName[GUID_LEN];
  generate(lpName);
  lock->mutex = CreateMutex(NULL, false, lpName);
#else
  pthread_mutex_init(&lock->mutex, NULL);
#endif
  return lock;
}

void DEventLock_lock(DEventLock *lock) {
#ifdef WIN32
  WaitForSingleObject(lock->mutex, INFINITE);
#else
  pthread_mutex_lock(&lock->mutex);
#endif
}

void DEventLock_unlock(DEventLock *lock) {
#ifdef WIN32
  ReleaseMutex(lock->mutex);
#else
  pthread_mutex_unlock(&lock->mutex);
#endif
}

void DEventLock_free(DEventLock *lock) {
#ifdef WIN32
  CloseHandle(lock->mutex);
#else
  pthread_mutex_destroy(&lock->mutex);
#endif
}

#endif