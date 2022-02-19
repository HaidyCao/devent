//
// Created by Haidy on 2021/11/6.
//

#ifndef DEVENT_BUFFER_H
#define DEVENT_BUFFER_H

#include <stdlib.h>
#ifndef _MSC_VER
#include <sys/unistd.h>
#include <sys/socket.h>
#endif

#include "def.h"
#include "c_linked_list.h"

#ifdef DEVENT_SSL
#include "openssl/ssl.h"
#include "openssl/err.h"

#endif

#ifndef DEVENT_BUFFER_SIZE
#define DEVENT_BUFFER_SIZE 4096
#endif

#ifndef DEVENT_READ_BUFFER_SIZE
#define DEVENT_READ_BUFFER_SIZE 40960
#endif

/**
 * create buffer
 * @return buffer
 */
DocketBuffer *DocketBuffer_new();

size_t DocketBuffer_length(DocketBuffer *buffer);

void DocketBuffer_free(DocketBuffer *buffer);

ssize_t DocketBuffer_write(DocketBuffer *buffer, const char *data, size_t len);

/**
 * read data from docket buffer
 * @param buffer buffer
 * @param data   data to put
 * @param len    data length
 * @return length of data read from buffer
 */
ssize_t DocketBuffer_read(DocketBuffer *buffer, char *data, size_t len);

/**
 * read data from docket buffer
 * @param buffer buffer
 * @param data   data to put
 * @param len    data length
 * @return -1 or len
 */
ssize_t DocketBuffer_read_full(DocketBuffer *buffer, char *data, size_t len);

/**
 * peek data from buffer
 * @param buffer buffer
 * @param data data to put
 * @param len data length
 * @return length of data read from buffer
 */
ssize_t DocketBuffer_peek(DocketBuffer *buffer, char *data, size_t len);

/**
 * peek data from buffer
 * @param buffer buffer
 * @param data data to put
 * @param len data length
 * @return -1 or len
 */
ssize_t DocketBuffer_peek_full(DocketBuffer *buffer, char *data, size_t len);

ssize_t DocketBuffer_send(DocketBuffer *buffer, SOCKET fd, int flags, struct sockaddr *address, socklen_t socklen
#ifdef DEVENT_SSL
        ,SSL *ssl
#endif
);

/**
 * move buffer
 * @param dest
 * @param from
 * @return length of data moved
 */
size_t DocketBuffer_moveto(DocketBuffer *dest, DocketBuffer *from);

#endif //DEVENT_BUFFER_H
