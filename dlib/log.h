
#ifndef H_SOCKS_LOG
#define H_SOCKS_LOG

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <time.h>

#define LIB_LOG_TRACE -1
#define LIB_LOG_DEBUG 0
#define LIB_LOG_INFO 1
#define LIB_LOG_WARNING 2
#define LIB_LOG_ERROR 3

void set_log_level(int level);

int get_log_level();

#ifdef __ANDROID__
#include <android/log.h>
void alog(int level, const char *fmt, ...);

#define LOGT(fmt, ...) alog(LIB_LOG_TRACE, "[%s(%d):%s]: " fmt "\n", basename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define LOGD(fmt, ...) alog(LIB_LOG_DEBUG, "[%s(%d):%s]: " fmt "\n", basename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define LOGI(fmt, ...) alog(LIB_LOG_INFO, "[%s(%d):%s]: " fmt "\n", basename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define LOGE(fmt, ...) alog(LIB_LOG_ERROR, "[%s(%d):%s]: " fmt "\n", basename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define LOGW(fmt, ...) alog(LIB_LOG_WARNING, "[%s(%d):%s]: " fmt "\n", basename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
void slog(int level, const char *fmt, ...);

#define LOGT(fmt, ...) slog(LIB_LOG_TRACE, "[%s(%d):%s]: " fmt "\n", basename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define LOGD(fmt, ...) slog(LIB_LOG_DEBUG, "[%s(%d):%s]: " fmt "\n", basename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define LOGI(fmt, ...) slog(LIB_LOG_INFO, "[%s(%d):%s]: " fmt "\n", basename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define LOGE(fmt, ...) slog(LIB_LOG_ERROR, "[%s(%d):%s]: " fmt "\n", basename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define LOGW(fmt, ...) slog(LIB_LOG_WARNING, "[%s(%d):%s]: " fmt "\n", basename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)

#endif // __ANDROID__

#endif // H_SOCKS_LOG