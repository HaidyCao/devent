//
// Created by Haidy on 2021/11/6.
//

#ifndef DEVENT_BUFFER_H
#define DEVENT_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#ifdef WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#endif

#include "../win_def.h"
#include "def.h"

#ifdef DEVENT_SSL
#include "openssl/ssl.h"
#include "openssl/err.h"

#endif

#ifndef DEVENT_BUFFER_SIZE
#define DEVENT_BUFFER_SIZE 10240
#endif

#ifndef MAX_POOL_SIZE
#define MAX_POOL_SIZE 1000
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

ssize_t DocketBuffer_send(DocketEvent *event, SOCKET fd, int flags, DocketBuffer *buffer);

/**
 * move buffer
 * @param dest
 * @param from
 * @return length of data moved
 */
size_t DocketBuffer_moveto(DocketBuffer *dest, DocketBuffer *from);

#ifdef __cplusplus
}
#endif

#endif //DEVENT_BUFFER_H
