//
// Created by Haidy on 2021/11/7.
//

#include "auth.h"
#include "server_context.h"
#include "log.h"
#include "docks_def.h"
#include "docks_connect.h"

void docks_on_auth_read(DocketEvent *event, void *ctx) {
  LOGD("");
  ServerContext *server = ctx;
  DocketBuffer *buffer = DocketEvent_get_in_buffer(event);

  if (DocketBuffer_length(buffer) < 2) {
    LOGD("need read more data");
    return;
  }

  char data[1024];
  ssize_t len = DocketBuffer_peek(buffer, data, sizeof(data));

  if (data[0] != SOCKS_USERNAME_PASSWORD_AUTH_VERSION_1) {
    LOGW("bad version: %d", data[0]);

    DocketEvent_free(event);
    ServerContext_free(server);
    return;
  }

  size_t username_len = (size_t) data[1];
  if (len - 2 < username_len) {
    LOGD("wait more data for username");
    return;
  }

  char u[username_len + 1];
  bzero(u, sizeof(u));
  memcpy(u, data + 2, username_len);

  LOGD("user = %s", u);

  size_t pwd_len = (size_t) data[2 + username_len];
  if (len - 2 - username_len - 1 < pwd_len) {
    LOGD("wait more data for password");
    return;
  }

  char p[pwd_len + 1];
  bzero(p, sizeof(p));
  memcpy(p, data + 2 + username_len + 1, pwd_len);
  DocketBuffer_read_full(buffer, data, 2 + username_len + 1 + pwd_len);

//  auth
  ServerConfig *config = server->config;
  char *pwd = c_hash_map_get(config->users, u);
  LOGD("pwd = %s", pwd);

  unsigned char auth_result = 0xFF;
  if (pwd && strcmp(pwd, p) == 0) {
    auth_result = SOCKS_USERNAME_PASSWORD_AUTH_SUCCESS;
  }

  char resp[2] = {SOCKS_USERNAME_PASSWORD_AUTH_VERSION_1, (char) auth_result};
  DocketEvent_write(event, resp, 2);

  DocketEvent_set_read_cb(event, docks_connect_read, ctx);
}

// client

void docks_on_read_UP(DocketEvent *event, void *ctx) {
  DocketBuffer *in = DocketEvent_get_in_buffer(event);
  if (DocketBuffer_length(in) < 2) {
    LOGD("need read more data");
    return;
  }

  char data[2];
  DocketBuffer_read_full(in, data, 2);

  if (data[0] != SOCKS_USERNAME_PASSWORD_AUTH_VERSION_1) {
    LOGI("bad version: %X", data[0]);
    // TODO close client
    return;
  }

  if (data[1] != SOCKS_USERNAME_PASSWORD_AUTH_SUCCESS) {
    LOGI("auth failed: %X", data[1]);
    // TODO close client
    return;
  }

  // send remote address to server
  docks_send_remote_address(event, ctx);
}