//
// Created by Haidy on 2021/12/5.
//

#ifndef DOCKET_DEVENT_UTILS_INTERNAL_H
#define DOCKET_DEVENT_UTILS_INTERNAL_H

#ifdef WIN32
#include <winnt.h>
#else
#include <stdint.h>
#endif
#include "utils.h"
#include "def.h"

/**
 * socket add flags
 * @param fd fd
 * @param flags flags
 * @return -1 failed
 */
int devent_turn_on_flags(int fd, int flags);

/**
 * 修改属性
 */
#define DEVENT_MOD_MOD 0
/**
 * 删除属性
 */
#define DEVENT_MOD_DEL 1
/**
 * 添加属性
 */
#define DEVENT_MOD_ADD 2

/**
 * update event status
 * @param dfd docket fd
 * @param fd event fd
 * @param events event: read or write
 * @param mod
 * @see DEVENT_MOD_MOD
 */
int devent_update_events(
#ifdef WIN32
    HANDLE handle,
#else
    int dfd,
#endif
    SOCKET fd, unsigned int events, int mod);

void devent_close_internal(DocketEvent *event, int what);

bool errno_is_EAGAIN(int eno);

char *docket_memdup(const char *data, unsigned int len);

#endif //DOCKET_DEVENT_UTILS_INTERNAL_H
