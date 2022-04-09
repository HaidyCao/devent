//
// Created by Haidy on 2021/12/20.
//

#include "client_config.h"
#include "log.h"
#include "def.h"
#include "clib.h"

#include "../getopt.h"
#include <stdio.h>
#include <string.h>
#ifndef WIN32
#include <sys/socket.h>
#endif

#define VAL_SSL_KEY_PATH 1
#define VAL_SSL_CERT_PATH 2

#ifdef WIN32
#if !defined(__MINGW32__) && !defined(__MINGW64__)
static int strcasecmp(char *str1, char *str2) {
  return _stricmp(str1, str2);
}
#endif

#endif

static const struct option long_options[] = {
    {"address", required_argument, NULL, 'a'},
    {"remote_host", required_argument, NULL, 'H'},
    {"remote_port", required_argument, NULL, 'P'},
    {"username", required_argument, NULL, 'u'},
    {"password", required_argument, NULL, 'p'},
    {"ssl", no_argument, NULL, 's'},
    {"log", required_argument, NULL, 'l'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0},
};

static void docks_usage() {
  printf("docks-client usage: \n"
         "-a  --address         : bind address\n"
         "-H  --remote_host     : remote address\n"
         "-P  --remote_post     : remote address\n"
         "-u  --username        : username\n"
         "-p  --password        : password\n"
         "-s  --ssl             : use ssl\n"
         "-l  --log             : log level: debug, info or error\n"
         "-h  --help            : show this help\n");
}

int init_client_config(ClientConfig *config, int argc, char **args) {
  int opt;
  while ((opt = getopt_long(argc, args, "a:H:P:u:p:sl:h", long_options, NULL)) != -1) {
    switch (opt) {
      case 'a':config->address = strdup(optarg);
        break;
      case 'H':config->remote_host = strdup(optarg);
        break;
      case 'P':config->remote_port = strtol(optarg, NULL, 10);
        break;
      case 'u':config->username = strdup(optarg);
        break;
      case 'p': config->password = strdup(optarg);
        break;
      case 's':config->ssl = true;
        break;
      case 'l':
        if (strcasecmp(optarg, "debug") == 0) {
          set_log_level(LIB_LOG_DEBUG);
        } else if (strcasecmp(optarg, "info") == 0) {
          set_log_level(LIB_LOG_INFO);
        } else if (strcasecmp(optarg, "error") == 0) {
          set_log_level(LIB_LOG_ERROR);
        }
        break;
      case 'h':docks_usage();
      default:return -1;
    }
  }

  if (config->address == NULL) {
    LOGE("address is empty");
    docks_usage();
    return -1;
  }

  bool username_is_empty = STR_IS_EMPTY(config->username);
  bool password_is_empty = STR_IS_EMPTY(config->password);
  if ((username_is_empty && !password_is_empty) || (!username_is_empty && password_is_empty)) {
    LOGE("username password must all empty or not empty");
    docks_usage();
    return -1;
  }

  return 0;
}