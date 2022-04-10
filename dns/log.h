
#ifndef H_SOCKS_LOG
#define H_SOCKS_LOG

#ifdef WIN32
const char *basename(const char *path);
#else
#include <libgen.h>
#endif

#define LIB_LOG_TRACE (-1)
#define LIB_LOG_DEBUG 0
#define LIB_LOG_INFO 1
#define LIB_LOG_WARNING 2
#define LIB_LOG_ERROR 3

void set_dns_log_level(int level);
int get_dns_log_level(void);

#define set_log_level set_dns_log_level
#define get_log_level get_dns_log_level

#if defined(EVENT_LOG_DISABLED)
#define LOGT(fmt, ...)

#define LOGD(fmt, ...)

#define LOGI(fmt, ...)

#define LOGE(fmt, ...)

#define LOGW(fmt, ...)
#else
void dns_log(int level, const char *fmt, ...);

#define LOGT(fmt, ...) dns_log(LIB_LOG_TRACE, "[%s(%d):%s]: " fmt "\n", basename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define LOGD(fmt, ...) dns_log(LIB_LOG_DEBUG, "[%s(%d):%s]: " fmt "\n", basename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define LOGI(fmt, ...) dns_log(LIB_LOG_INFO, "[%s(%d):%s]: " fmt "\n", basename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define LOGE(fmt, ...) dns_log(LIB_LOG_ERROR, "[%s(%d):%s]: " fmt "\n", basename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define LOGW(fmt, ...) dns_log(LIB_LOG_WARNING, "[%s(%d):%s]: " fmt "\n", basename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)

#endif // __ANDROID__

#endif // H_SOCKS_LOG