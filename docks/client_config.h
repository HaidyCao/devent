//
// Created by Haidy on 2021/12/20.
//

#ifndef DOCKET_DOCKS_CLIENT_CONFIG_H
#define DOCKET_DOCKS_CLIENT_CONFIG_H

#include <stdbool.h>

typedef struct client_config {
  char *address;
  char *username;
  char *password;
  char *remote_host;
  unsigned short remote_port;

  bool ssl;
} ClientConfig;

int init_client_config(ClientConfig *config, int argc, char **args);

#endif //DOCKET_DOCKS_CLIENT_CONFIG_H
