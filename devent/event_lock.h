//
// Created by haidy on 2022/4/9.
//

#ifndef DOCKET_DEVENT_EVENT_LOCK_H_
#define DOCKET_DEVENT_EVENT_LOCK_H_
#ifdef DEVENT_MULTI_THREAD

#ifdef WIN32
typedef void *HANDLE;
#else
#include <pthread.h>
#endif

typedef struct {
#ifdef WIN32
  HANDLE *mutex;
#else
  pthread_mutex_t mutex;
#endif
} DEventLock;

DEventLock *DEventLock_new();

void DEventLock_lock(DEventLock *lock);

void DEventLock_unlock(DEventLock *lock);

void DEventLock_free(DEventLock *lock);

#endif

#endif //DOCKET_DEVENT_EVENT_LOCK_H_
