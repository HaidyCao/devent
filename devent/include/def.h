//
// Created by Haidy on 2021/11/6.
//

#ifndef DEVENT_DEF_H
#define DEVENT_DEF_H

//#define DEVENT_MULTI_THREAD
#ifndef WIN32
typedef int SOCKET;
#endif

typedef struct docket_event DocketEvent;

/**
 * define Docket struct
 */
typedef struct docket Docket;

typedef struct docket_listener DocketListener;

typedef struct docket_buffer DocketBuffer;

#endif //DEVENT_DEF_H
