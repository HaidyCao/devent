//
// Created by Haidy on 2021/11/6.
//

#ifndef DEVENT_DEF_H
#define DEVENT_DEF_H

#ifdef __cplusplus
extern "C" {
#endif

//#define DEVENT_MULTI_THREAD
#ifndef WIN32
typedef int SOCKET;
#endif
#include <stdbool.h>

#define DEVENT_LOG_DEBUG 0

#define DEVENT_LOG_INFO 1

#define DEVENT_LOG_ERROR 2

typedef struct docket_event DocketEvent;

/**
 * define Docket struct
 */
typedef struct docket Docket;

typedef struct docket_listener DocketListener;

typedef struct docket_buffer DocketBuffer;

typedef bool (*docket_fn_custom_dns_server)(struct sockaddr **p_sockaddr, unsigned int *len);

#ifdef __cplusplus
}
#endif

#endif //DEVENT_DEF_H
