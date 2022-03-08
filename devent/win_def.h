//
// Created by haidy on 2022/2/13.
//

#ifndef DOCKET_DEVENT_WIN_DEF_H_
#define DOCKET_DEVENT_WIN_DEF_H_

#ifdef WIN32
#include <stdio.h>
#include <basetsd.h>

#define bzero(p, s) memset(p, 0, s)
typedef SSIZE_T ssize_t;

#endif

#endif //DOCKET_DEVENT_WIN_DEF_H_
