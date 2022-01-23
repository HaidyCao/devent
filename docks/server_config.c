//
// Created by Haidy on 2021/11/7.
//
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "server_config.h"
#include "log.h"
#include "docks_def.h"
#include "clib.h"

#define VAL_SSL_KEY_PATH 1
#define VAL_SSL_CERT_PATH 2

static const struct option long_options[] = {
    {"address", required_argument, NULL, 'a'},
    {"users", required_argument, NULL, 'u'},
    {"ssl", no_argument, NULL, 's'},
    {"ssl_key_path", required_argument, NULL, VAL_SSL_KEY_PATH},
    {"ssl_cert_path", required_argument, NULL, VAL_SSL_CERT_PATH},
    {"log", no_argument, NULL, 'l'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0},
};

static void docks_usage() {
  printf("docks usage: \n"
         "-a  --address       : bind address\n"
         "-u  --users         : user config file path\n"
         "-s  --ssl           : use ssl\n"
         "--ssl_key_path      : ssl key path\n"
         "--ssl_cert_path     : ssl cert path\n"
         "-l  --log           : log level: debug, info or error\n"
         "-h  --help          : show this help\n");
}

#define MAX_USER_LEN 256
#define MAX_PWD_LEN 256
#define MAX_LINE_LEN 1024

static void parse_users(const char *path, ServerConfig *config) {
  FILE *file = fopen(path, "r");
  if (file == NULL) {
    return;
  }
  config->users = c_hash_map_new();

  char line[MAX_LINE_LEN];
  while (fgets(line, sizeof(line), file) != NULL) {
    LOGD("read line: %s", line);

    size_t len = strlen(line);
    if (len >= 2 && line[len - 2] == '\r' && line[len - 1] == '\n') {
      line[len - 2] = '\0';
    } else if (len >= 1 && line[len - 1] == '\n') {
      line[len - 1] = '\0';
    }

    len = strlen(line);
    if (len == 0) {
      continue;
    }

    char *pwd = strchr(line, ' ');
    if (pwd == NULL) {
      LOGD("read failed: %s", line);
      continue;
    }

    size_t user_len = pwd - line;
    if (user_len > MAX_USER_LEN) {
      LOGW("username length > %d", MAX_USER_LEN);
      continue;
    }

    char user[MAX_USER_LEN];
    bzero(user, sizeof(user));
    strncpy(user, line, pwd - line);

    while (pwd[0] == ' ') {
      pwd++;
    }

    if (strlen(pwd) > MAX_PWD_LEN) {
      LOGW("password length > %d", MAX_USER_LEN);
      continue;
    }

    if (strchr(pwd, ' ')) {
      LOGW("password has space");
      continue;
    }

    LOGD("add account: [%s: %s]", user, pwd);
    c_hash_map_put(config->users, user, strdup(pwd));
  }

  fclose(file);
}

int parse_config(int argc, char **args, ServerConfig *config) {
  int opt;
  while ((opt = getopt_long(argc, args, "a:u:slh", long_options, NULL)) != -1) {
    switch (opt) {
    case 'a':config->address = strdup(optarg);
      break;
    case 'u':parse_users(optarg, config);
      break;
    case 's':config->ssl = true;
      break;
    case VAL_SSL_KEY_PATH:config->ssl_key_path = strdup(optarg);
      break;
    case VAL_SSL_CERT_PATH:config->ssl_cert_path = strdup(optarg);
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
      break;
    default:break;
    }
  }

  if (config->address == NULL) {
    LOGE("address is empty");
    docks_usage();
    return -1;
  }

  if (config->ssl && (STR_IS_EMPTY(config->ssl_key_path) || STR_IS_EMPTY(config->ssl_cert_path))) {
    LOGE("ssl_key_path or ssl_cert_path is empty");
    docks_usage();
    return -1;
  }

  config->bind_type = SOCKS5_ATYPE_IPV4;

  config->bind_address = malloc(IPV4_LEN);
  in_addr_t ip = inet_addr("127.0.0.1");
  n_write_uint32_t_to_data(config->bind_address, ntohl(ip), 0);
  config->bind_address_length = IPV4_LEN;

  if (c_hash_map_get_count(config->users) == 0) {
    LOGE("users is empty");
    docks_usage();
    return -1;
  }
  return 0;
}