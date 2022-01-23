//
// Created by Haidy on 2021/11/7.
//

#include "method.h"
#include "log.h"
#include "docks_def.h"
#include "server_context.h"
#include "negotication.h"
#include "auth.h"
#include "docks_connect.h"

static void docks_on_read_methods_inner(DocketEvent *event, void *ctx) {
  ServerContext *context = ctx;
  size_t len = DocketBuffer_length(DocketEvent_get_in_buffer(event));

  if (len < context->method_count) {
    LOGI("socks_read_cb not received all methods, wait");
    return;
  }

  char methods[context->method_count];
  if (DocketEvent_read_full(event, methods, context->method_count) != context->method_count) {
    LOGI("read methods failed");
    DocketEvent_free(event);
    ServerContext_free(context);
    return;
  }

  int i;
  for (i = 0; i < context->method_count; i++) {
    LOGD("client supported method[%d:%d] = %x", i, methods[i], context->method_count);
  }
  char method = docks_negotication(context, methods, context->method_count);

  char methods_resp[2];
  methods_resp[0] = SOCKS5_VERSION;
  methods_resp[1] = method;

  DocketEvent_write(event, methods_resp, 2);
  context->method = method;
  LOGD("method = %x", method);

  if ((unsigned char) method == SOCKS5_METHOD_NO_AUTHENTICATION_REQUIRED) {
    DocketEvent_set_read_cb(event, docks_connect_read, ctx);
  } else if (method == SOCKS5_METHOD_USERNAME_PASSWORD) {
    DocketEvent_set_read_cb(event, docks_on_auth_read, ctx);
  } else {
    LOGI("method [%x] is not support", method);
    DocketEvent_free(event);
    ServerContext_free(context);
  }
}

void docks_on_read_methods(DocketEvent *event, void *ctx) {
  ServerContext *context = ctx;
  size_t len = DocketBuffer_length(DocketEvent_get_in_buffer(event));
  if (len < 2) {
    LOGD("need more data");
    return;
  }

  char version_method[2];
  DocketEvent_read_full(event, version_method, sizeof(version_method));
  if (version_method[0] != SOCKS5_VERSION) {
    LOGI("bad version: %d", version_method[0]);
    ServerContext_free(context);
    DocketEvent_free(event);
    return;
  }

  unsigned char method_count = version_method[1];
  context->method_count = method_count;

  DocketEvent_set_read_cb(event, docks_on_read_methods_inner, ctx);
  docks_on_read_methods_inner(event, ctx);
}

// client

static void on_read_negotication(DocketEvent *event, void *ctx) {
  DocketBuffer *in = DocketEvent_get_in_buffer(event);
  if (DocketBuffer_length(in) < 2) {
    LOGD("need read more");
    return;
  }

  char data[2];
  DocketBuffer_read_full(in, data, 2);
  LOGD("%X, %X", data[0], data[1]);

  if (data[0] != SOCKS5_VERSION) {
    LOGI("bad version: %X", data[0]);
    // TODO close client
    return;
  }
  LOGD("%X", ((ClientContext *) ctx)->on_server_connect);

  ClientContext *context = ctx;
  if (data[1] == SOCKS5_METHOD_USERNAME_PASSWORD) {
    char auth_header[2] = {SOCKS_USERNAME_PASSWORD_AUTH_VERSION_1, (char) strlen(context->config->username)};
    DocketEvent_write(event, auth_header, 2);
    DocketEvent_write(event, context->config->username, auth_header[1]);
    char pwd_len[1] = {(char) strlen(context->config->password)};
    DocketEvent_write(event, pwd_len, 1);
    DocketEvent_write(event, context->config->password, pwd_len[0]);

    DocketEvent_set_read_cb(event, docks_on_read_UP, ctx);
  } else if (data[1] == SOCKS5_METHOD_NO_AUTHENTICATION_REQUIRED) {
    docks_send_remote_address(event, ctx);
  } else {
    // TODO close client
    return;
  }
}

void docks_write_methods(ClientContext *context) {
  DocketEvent *event = context->event;
  LOGD("%X", context->on_server_connect);

  char data[4] = {SOCKS5_VERSION};
  if (STR_IS_EMPTY(context->config->username)) {
    data[1] = 1;
    data[2] = SOCKS5_METHOD_NO_AUTHENTICATION_REQUIRED;
    LOGD("write method %X", SOCKS5_METHOD_NO_AUTHENTICATION_REQUIRED);
  } else {
    data[1] = 2;
    data[2] = SOCKS5_METHOD_NO_AUTHENTICATION_REQUIRED;
    data[3] = SOCKS5_METHOD_USERNAME_PASSWORD;
    LOGD("write method %X, %X", SOCKS5_METHOD_NO_AUTHENTICATION_REQUIRED, SOCKS5_METHOD_USERNAME_PASSWORD);
  }

  size_t data_len = data[1] + 2;
  DocketEvent_write(event, data, data_len);
  LOGD("%X", context->on_server_connect);

  DocketEvent_set_write_cb(event, NULL, context);
  DocketEvent_set_read_cb(event, on_read_negotication, context);
}