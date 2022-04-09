#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef WIN32
#include <WS2tcpip.h>
typedef SSIZE_T ssize_t;
#else
#include <netdb.h>
#include <strings.h>
#endif

#include "../dlib/clib.h"
#include "c_dns.h"

#define LOGD(fmt, ...) printf("[%s(%d):%s]: " fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

static void hexDump(char *buf, int len, int addr) {
  int i, j, k;
  char binstr[80];

  for (i = 0; i < len; i++) {
    if (0 == (i % 16)) {
      sprintf(binstr, "%08x -", i + addr);
      sprintf(binstr, "%s %02x", binstr, (unsigned char) buf[i]);
    } else if (15 == (i % 16)) {
      sprintf(binstr, "%s %02x", binstr, (unsigned char) buf[i]);
      sprintf(binstr, "%s  ", binstr);
      for (j = i - 15; j <= i; j++) {
        sprintf(binstr, "%s%c", binstr, ('!' < buf[j] && buf[j] <= '~') ? buf[j] : '.');
      }
      printf("%s\n", binstr);
    } else {
      sprintf(binstr, "%s %02x", binstr, (unsigned char) buf[i]);
    }
  }
  if (0 != (i % 16)) {
    k = 16 - (i % 16);
    for (j = 0; j < k; j++) {
      sprintf(binstr, "%s   ", binstr);
    }
    sprintf(binstr, "%s  ", binstr);
    k = 16 - k;
    for (j = i - k; j < i; j++) {
      sprintf(binstr, "%s%c", binstr, ('!' < buf[j] && buf[j] <= '~') ? buf[j] : '.');
    }
    printf("%s\n", binstr);
  }
}

static int test_parse_a() {
  char data[65535];
  size_t len = sizeof(data);
  ssize_t pl;
  if ((pl = c_dns_pack("www.baidu.com", data, len, C_DNS_QTYPE_A)) == -1) {
    LOGD("pack dns data failed");
    return -1;
  }

  struct sockaddr addr;
  socklen_t addr_len = sizeof(addr);

  if (parse_address("114.114.114.114:53", &addr, &addr_len) == -1) {
    LOGD("parse address failed");
    return -1;
  }

  int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sendto(fd, data, pl, 0, &addr, addr_len) == -1) {
    LOGD("sendto failed: err = %s", strerror(errno));
    return -1;
  }

  char rdata[65535];
  ssize_t rlen = recvfrom(fd, rdata, sizeof(rdata), 0, &addr, &addr_len);
  if (rlen < 0) {
    LOGD("recvfrom failed: err = %s", strerror(errno));
    return -1;
  }
  LOGD("rlen = %ld", rlen);

  struct hostent *host;
  c_dns_parse_a(rdata, rlen, &host);

  return 0;
}

static int test_ipv4_cb(char *domain, struct in_addr *ip) {
  ip->s_addr = inet_addr("127.0.0.3");
  return sizeof(struct in_addr);
}

static int test_ipv6_cb(char *domain, struct in6_addr *ip) {
  if (inet_pton(AF_INET6, "::3", ip) == -1)
    return -1;
  return sizeof(struct in6_addr);
}

static int test_custom_ip() {
  char data[65535];
  size_t len = sizeof(data);
  ssize_t pl;
  if ((pl = c_dns_pack("www.baidu.com", data, len, C_DNS_QTYPE_A)) == -1) {
    LOGD("pack dns data failed");
    return -1;
  }

  char *resp_data = NULL;
  socklen_t resp_len = 0;
  if (c_dns_gen_inet_response(data, pl, &resp_data, &resp_len, test_ipv4_cb, test_ipv6_cb) == -1) {
    LOGD("c_dns_gen_inet_response failed");
    return -1;
  }

  LOGD("parse success: resp len = %zu", resp_len);
  hexDump(resp_data, resp_len, 0);

  struct hostent *host;
  c_dns_parse_a(resp_data, resp_len, &host);

  return 0;
}

static void test_sscanf() {
  int ipv4[4];
  int r = sscanf("127.0.0.l", "%d.%d.%d.%d", &ipv4[0], &ipv4[1], &ipv4[2], &ipv4[3]);
  LOGD("r = %d, ipv4[0] = %d, ipv4[1] = %d, ipv4[2] = %d, ipv4[3] = %d", r, ipv4[0], ipv4[1], ipv4[2], ipv4[3]);
}

int main(int argc, char **argv) {
//  test_custom_ip();
#ifdef WIN32
  WSADATA lpWsaData;
  int startUpResult = WSAStartup(WINSOCK_VERSION, &lpWsaData);
  if (startUpResult != 0) {
    int l = WSAGetLastError();
    return -1;
  }
#endif

  test_parse_a();
//  test_sscanf();
  return 0;
}