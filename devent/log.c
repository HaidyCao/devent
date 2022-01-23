//
// Created by Haidy on 2021/10/31.
//

#include <sys/time.h>
#include <stdio.h>
#include <stdarg.h>

#include "log.h"

static int s_log_level = DEVENT_LOG_DEBUG;

void devent_set_log_level(int level) {
    s_log_level = level;
}

static const char *log_level_to_string(int level) {
    switch (level) {
        case DEVENT_LOG_DEBUG:
            return "DEBUG";
        case DEVENT_LOG_INFO:
            return "INFO";
        case DEVENT_LOG_ERROR:
            return "ERROR";
        default:
            return "unknown";
    }
}

void devent_log(int level, const char *fmt, ...) {
    if (level < s_log_level) {
        return;
    }

    time_t now_time = time(NULL);
    struct tm *l = localtime(&now_time);

    struct timeval t;
    gettimeofday(&t, NULL);

    printf("%04d-%02d-%02d %02d:%02d:%02d %03d %s: ", l->tm_year + 1900, l->tm_mon + 1, l->tm_mday, l->tm_hour,
           l->tm_min, l->tm_sec, (int) (t.tv_usec / 1000), log_level_to_string(level));

    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);

    va_end(ap);
}