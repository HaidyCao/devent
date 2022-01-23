//
// Created by Haidy on 2021/10/31.
//

#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>

#ifndef DEVENT_LOG_H
#define DEVENT_LOG_H

#define DEVENT_LOG_DEBUG 0

#define DEVENT_LOG_INFO 1

#define DEVENT_LOG_ERROR 2

void devent_set_log_level(int level);

void devent_log(int level, const char *fmt, ...);

#define LOGD(fmt, ...) devent_log(DEVENT_LOG_DEBUG, "[%s(%d):%s]: " fmt "\n", basename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define LOGI(fmt, ...) devent_log(DEVENT_LOG_INFO, "[%s(%d):%s]: " fmt "\n", basename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define LOGE(fmt, ...) devent_log(DEVENT_LOG_ERROR, "[%s(%d):%s]: " fmt "\n", basename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define TODO(msg) printf(msg); exit(-1)


#endif //DEVENT_LOG_H
