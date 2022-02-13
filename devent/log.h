//
// Created by Haidy on 2021/10/31.
//

#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
#include <string.h>
#define filename(x) strrchr(x,'\\')?strrchr(x,'\\')+1:x
#else
#include <libgen.h>
#define filename(f) basename(f)
#endif

#ifndef DEVENT_LOG_H
#define DEVENT_LOG_H

#define DEVENT_LOG_DEBUG 0

#define DEVENT_LOG_INFO 1

#define DEVENT_LOG_ERROR 2

void devent_set_log_level(int level);

void devent_log(int level, const char *fmt, ...);

#define LOGD(fmt, ...) devent_log(DEVENT_LOG_DEBUG, "[%s(%d):%s]: " fmt "\n", filename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define LOGI(fmt, ...) devent_log(DEVENT_LOG_INFO, "[%s(%d):%s]: " fmt "\n", filename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define LOGE(fmt, ...) devent_log(DEVENT_LOG_ERROR, "[%s(%d):%s]: " fmt "\n", filename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define TODO(msg) printf(msg); exit(-1)


#endif //DEVENT_LOG_H
