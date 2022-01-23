//
// Created by Haidy on 2021/11/7.
//
#include "clib.h"
#include "log.h"
#include "server_config.h"
#include "docket.h"
#include "listener.h"
#include "server.h"

int main(int argc, char **args) {
  ServerConfig config;
  bzero(&config, sizeof(config));
  signal(SIGPIPE, SIG_IGN);

  if (parse_config(argc, args, &config) == -1) {
    return -1;
  }

  struct sockaddr_storage local;
  socklen_t socklen = sizeof(local);
  const char *address = "127.0.0.1:1188";
  if (parse_address(config.address, (struct sockaddr *) &local, &socklen) == -1) {
    LOGE("parse address failed: %s", address);
    return -1;
  }

  Docket *docket = Docket_new();
  Docket_set_dns_server(docket, "114.114.114.114");

  if (config.ssl) {
    Docket_create_ssl_listener_by_path(docket,
                                       config.ssl_cert_path,
                                       config.ssl_key_path,
                                       NULL,
                                       docks_on_socks_connect,
                                       -1,
                                       (struct sockaddr *) &local,
                                       socklen,
                                       &config);
  } else {
    Docket_create_listener(docket, docks_on_socks_connect, -1, (struct sockaddr *) &local, socklen, &config);
  }

  return Docket_loop(docket);
}