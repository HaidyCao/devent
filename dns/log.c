#include "log.h"

#include <stdarg.h>
#include <stdio.h>

static int log_level = 0;

#ifdef __ANDROID__
#include <android/log.h>

void alog(int level, const char *fmt, ...) {
    if (level < log_level) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    int l = ANDROID_LOG_DEBUG;
    if (level == LIB_LOG_INFO) {
        l = ANDROID_LOG_INFO;
    } else if (level == LIB_LOG_WARNING) {
        l = ANDROID_LOG_WARN;
    } else if (level == LIB_LOG_ERROR) {
        l = ANDROID_LOG_ERROR;
    }
    __android_log_vprint(l, "event", fmt, ap);

    va_end(ap);
}
#endif

const char *log_level_to_string(int level) {
  switch (level) {
    case LIB_LOG_TRACE:return "TRACE";
    case LIB_LOG_DEBUG:return "DEBUG";
    case LIB_LOG_INFO:return "INFO";
    case LIB_LOG_ERROR:return "ERROR";
    case LIB_LOG_WARNING:return "WARNING";
    default:return "unknown";
  }
}

void set_dns_log_level(int level) {
  log_level = level;
  if (log_level > LIB_LOG_WARNING)
    log_level = LIB_LOG_WARNING;
  else if (log_level < LIB_LOG_DEBUG)
    log_level = LIB_LOG_DEBUG;
}

int get_dns_log_level(void) {
  return log_level;
}

#include <time.h>
#ifdef WIN32
#include <sysinfoapi.h>
#else
#include <sys/time.h>
#endif

#ifdef WIN32
typedef long int suseconds_t;

struct timeval {
  time_t tv_sec;
  suseconds_t tv_usec;
};

static int gettimeofday(struct timeval *tp, void *tzp) {
  time_t clock;
  struct tm tm;
  SYSTEMTIME wtm;
  GetLocalTime(&wtm);
  tm.tm_year = wtm.wYear - 1900;
  tm.tm_mon = wtm.wMonth - 1;
  tm.tm_mday = wtm.wDay;
  tm.tm_hour = wtm.wHour;
  tm.tm_min = wtm.wMinute;
  tm.tm_sec = wtm.wSecond;
  tm.tm_isdst = -1;
  clock = mktime(&tm);
  tp->tv_sec = clock;
  tp->tv_usec = wtm.wMilliseconds * 1000;
  return (0);
}

const char *basename(const char *path) {
  char *name = strrchr(path, '\\');
  if (name == NULL) {
    return path;
  }
  return name;
}
#endif

void dns_log(int level, const char *fmt, ...) {
  if (level < log_level) {
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
