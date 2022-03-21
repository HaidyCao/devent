//
// Created by Haidy on 2021/11/20.
//
#include <stddef.h>
#include <stdio.h>

#include "utils.h"
#include "utils_test.h"

#define LOGD(fmt, ...) printf("[%s(%d):%s]: " fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

void test_devent_parse_ipv4() {
  char *ips[] = {"www.baidu.com", "a.b.c.d", NULL, "127.0.0.1", "256.0.0.1"};
  unsigned char result[4];
  char **ip = ips;
  while (*ip) {
    if (devent_parse_ipv4(*ip, (unsigned char *) &result)) {
      LOGD("解析成功");
    } else {
      LOGD("解析失败");
    }
    ip++;
  }
}