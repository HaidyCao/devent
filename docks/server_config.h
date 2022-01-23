//
// Created by Haidy on 2021/11/7.
//

#ifndef DOCKET_SERVER_CONFIG_H
#define DOCKET_SERVER_CONFIG_H

#include <stdbool.h>

#include "c_hash_map.h"
#include "def.h"

#define SERVER_BIND_TYPE_NONE -1
#define SERVER_BIND_TYPE_DOMAIN 1

struct server_config {
  /**
   * bind ip
   */
  char *address;

  CHashMap *users;

  int heartbeat_interval;
  int read_timeout;
  int write_timeout;

  /**
   * bind type
   */
  char bind_type;
  char bind_address_length;
  char *bind_address;

  char *ssl_key_path;
  char *ssl_cert_path;
  bool ssl;
};

int parse_config(int argc, char **args, ServerConfig *config);

#endif //DOCKET_SERVER_CONFIG_H
