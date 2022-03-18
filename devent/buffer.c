//
// Created by Haidy on 2021/11/6.
//
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <errno.h>

#ifndef WIN32
#include <sys/socket.h>
#include <unistd.h>
#endif

#ifdef DEVENT_MULTI_THREAD
#include <pthread.h>
#endif

#include "def.h"
#include "docket_def.h"
#include "event.h"
#include "event_def.h"
#include "buffer_def.h"
#include "c_linked_list.h"
#include "win_def.h"
#include "log.h"

struct docket_buffer {
  /**
   * buffer length
   */
  size_t length;

  CLinkedList *list;

#ifdef DEVENT_MULTI_THREAD
  pthread_mutex_t mutex;
#endif
};

#ifdef DEVENT_MULTI_THREAD
#define LOCK(buf, block)                \
    pthread_mutex_lock(&(buf)->mutex);    \
    block                               \
    pthread_mutex_unlock(&(buf)->mutex);

#else

#define LOCK(buffer, block) \
    block                   \

#endif

static CLinkedList *buffer_pool = NULL;

static Buffer *get_buffer_from_pool() {
  if (buffer_pool == NULL)
    buffer_pool = CLinkedList_new();

  Buffer *buffer = c_linked_list_remove_header(buffer_pool);
  if (buffer == NULL) {
    buffer = malloc(sizeof(Buffer));
  }
  buffer->len = 0;
  buffer->pos = 0;
  return buffer;
}

static void buffer_release(Buffer *buffer) {
  if (buffer_pool == NULL)
    buffer_pool = CLinkedList_new();

  if (c_linked_list_length(buffer_pool) <= MAX_POOL_SIZE) {
    c_linked_list_add(buffer_pool, buffer);
  } else {
    free(buffer);
  }
}

Buffer *Docket_buffer_alloc() {
  return get_buffer_from_pool();
}

void Docket_buffer_release(Buffer *buffer) {
  buffer_release(buffer);
}

DocketBuffer *DocketBuffer_new() {
  DocketBuffer *buffer = calloc(1, sizeof(DocketBuffer));
  buffer->length = 0;
  buffer->list = CLinkedList_new();

#ifdef DEVENT_MULTI_THREAD
  pthread_mutex_init(&buffer->mutex, NULL);
#endif

  return buffer;
}

size_t DocketBuffer_length(DocketBuffer *buffer) {
  return buffer->length;
}

void DocketBuffer_free(DocketBuffer *buffer) {
  LOCK(buffer, {
    c_linked_list_merge(buffer_pool, buffer->list);
    c_linked_list_free(buffer->list);
    buffer->list = NULL;
  })

#ifdef DEVENT_MULTI_THREAD
  pthread_mutex_destroy(&buffer->mutex);
#endif
}

ssize_t DocketBuffer_write(DocketBuffer *buffer, const char *data, size_t len) {
  if (buffer == NULL) {
    return -1;
  }

  if (data == NULL || len == 0) {
    return 0;
  }

  size_t left_size = len;
  size_t pos = 0;

  CLinkedList *list = c_linked_list_new();
  size_t write_len = 0;

  Buffer *mb = get_buffer_from_pool();
  c_linked_list_add(list, mb);
  do {
    size_t footer_left = sizeof(mb->data) - mb->pos - mb->len;
    size_t cpy_len = left_size > footer_left ? footer_left : left_size;

    memcpy(mb->data + mb->pos + mb->len, data + pos, cpy_len);
    mb->len += cpy_len;

    left_size -= cpy_len;
    pos += cpy_len;
    write_len += cpy_len;

    if (left_size == 0)
      break;
    mb = get_buffer_from_pool();
    c_linked_list_add(list, mb);
  } while (true);

  LOCK(buffer, {
    c_linked_list_merge(buffer->list, list);
    buffer->length += write_len;
  })
  c_linked_list_free(list);
  return (ssize_t) write_len;
}

static ssize_t DocketBuffer_read_internal(DocketBuffer *buffer, char *data, size_t len) {
  if (data == NULL || len == 0) {
    return -1;
  }

  size_t removed_size = 0;
  size_t left_size = len;
  Buffer *buf;
  while ((buf = c_linked_list_get_header(buffer->list))) {
    if (buf->len > left_size) {
      memcpy(data + removed_size, buf->data + buf->pos, left_size);

      buf->pos += left_size;
      buf->len -= left_size;
      buffer->length -= left_size;
      removed_size += left_size;
      break;
    }

    memcpy(data + removed_size, buf->data + buf->pos, buf->len);
    removed_size += buf->len;
    left_size -= buf->len;
    buffer->length -= buf->len;

    c_linked_list_remove_header(buffer->list);
    buffer_release(buf);
  }

  return (ssize_t) removed_size;
}

static ssize_t DocketBuffer_peek_internal(DocketBuffer *buffer, char *data, size_t len) {
  if (data == NULL || len == 0) {
    return -1;
  }

  size_t left_size = len;
  size_t pos = 0;

  void *it = c_linked_list_iterator(buffer->list);
  Buffer *mb;
  while (it != NULL) {
    mb = c_linked_list_iterator_get_value(it);
    if (mb == NULL || mb->len == 0) {
      it = c_linked_list_iterator_next(it);
      continue;
    }
    if (mb->len >= left_size) {
      memcpy(data + pos, mb->data + mb->pos, left_size);
      pos += left_size;
      break;
    }
    memcpy(data + pos, mb->data + mb->pos, mb->len);
    left_size -= mb->len;
    pos += mb->len;

    it = c_linked_list_iterator_next(it);
  }

  return (ssize_t) pos;
}

ssize_t DocketBuffer_read(DocketBuffer *buffer, char *data, size_t len) {
  ssize_t result;
  LOCK(buffer, {
    result = DocketBuffer_read_internal(buffer, data, len);
  })
  return result;
}

ssize_t DocketBuffer_read_full(DocketBuffer *buffer, char *data, size_t len) {
  ssize_t result;
  LOCK(buffer, {
    if (len > buffer->length) {
      return -1;
    }
    result = DocketBuffer_read_internal(buffer, data, len);
  })
  return result;
}

ssize_t DocketBuffer_peek(DocketBuffer *buffer, char *data, size_t len) {
  ssize_t result;
  LOCK(buffer, {
    result = DocketBuffer_peek_internal(buffer, data, len);
  })
  return result;
}

ssize_t DocketBuffer_peek_full(DocketBuffer *buffer, char *data, size_t len) {
  ssize_t result;
  LOCK(buffer, {
    if (len > buffer->length) {
      return -1;
    }
    result = DocketBuffer_peek_internal(buffer, data, len);
  })
  return result;
}

#ifdef WIN32
int DocketBuffer_win_send(IO_CONTEXT *io, int flags, struct sockaddr *address, socklen_t socklen) {
  if (address) {
    // TODO test for udp
    return WSASendTo(io->socket,
                     &io->buf,
                     1,
                     NULL,
                     flags, address,
                     socklen,
                     (LPWSAOVERLAPPED) io,
                     NULL);
  }
  return WSASend(io->socket, &io->buf, 1, NULL, flags, (LPWSAOVERLAPPED) io, NULL);
}
#endif

ssize_t DocketBuffer_send(DocketEvent *event, SOCKET fd, int flags, DocketBuffer *buffer) {
  Buffer *head = NULL;
  LOCK(buffer, {
    head = c_linked_list_get_header(buffer->list);
  })

  ssize_t wr;
#ifdef WIN32
  IO_CONTEXT *io = IO_CONTEXT_new(IOCP_OP_WRITE, fd);

  io->buf.buf = malloc(head->len);
  io->buf.len = head->len;
  memcpy(io->buf.buf, head->data + head->pos, head->len);
  wr = (ssize_t) head->len;

  if (DocketBuffer_win_send(io, flags, event->remote_address, event->remote_address_len) != 0) {
    int lastError = WSAGetLastError();
    if (lastError != WSA_IO_PENDING) {
      IO_CONTEXT_free(io);
      return -1;
    }
  }
#else
#ifdef DEVENT_SSL
    if (event->ssl) {
        wr = SSL_write(event->ssl, head->data + head->pos, (int) head->len);
    } else
#endif
    if (address) {
        wr = sendto(event->fd, head->data + head->pos, head->len, flags, event->remote_address, event->remote_address_len);
    } else {
        wr = send(event->fd, head->data + head->pos, head->len, flags);
    }

    if (wr == -1 && errno == ENOTSOCK) {
        wr = write(event->fd, head->data + head->pos, head->len);
    }

    if (wr == -1 || wr == 0) {
        return -1;
    }
#endif

  LOCK(buffer, {
    c_linked_list_remove_header(buffer->list);
    buffer->length -= head->len;
  })
  buffer_release(head);
  return wr;
}

size_t DocketBuffer_moveto(DocketBuffer *dest, DocketBuffer *from) {
  CLinkedList *data = NULL;
  size_t size;
  LOCK(from, {
    c_linked_list_move(&data, from->list);
    size = from->length;
    from->length = 0;
  })

  LOCK(dest, {
    c_linked_list_merge(dest->list, data);
    dest->length += size;
  })
  c_linked_list_free(data);
  return size;
}