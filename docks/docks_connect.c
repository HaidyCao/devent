//
// Created by Haidy on 2021/11/7.
//
#ifdef WIN32

#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "connect.h"
#include "docks_connect.h"
#include "server_context.h"
#include "docks_def.h"
#include "log.h"
#include "clib.h"
#include "remote.h"
#include "transfer.h"

#define SOCKS5_REPLY_SUCCEEDED (char)0x00
#define SOCKS5_REPLY_GENERAL_SOCKS_SERVER_FAILURE (char)0x01
#define SOCKS5_REPLY_CONNECTION_NOT_ALLOWED_BY_RULESET (char)0x02
#define SOCKS5_REPLY_NETWORK_UNREACHABLE (char)0x03
#define SOCKS5_REPLY_HOST_UNREACHABLE (char)0x04
#define SOCKS5_REPLY_CONNECT_REFUSED (char)0x05
#define SOCKS5_REPLY_TTL_EXPIRED (char)0x06
#define SOCKS5_REPLY_COMMAND_NOT_SUPPORTED (char)0x07
#define SOCKS5_REPLY_ADDRESS_TYPE_NOT_SUPPORTED (char)0x08

static uint16_t port_to_uint16(const char *data) {
  uint16_t a = (uint16_t) ((uint16_t) ((uint16_t) (data[0] << (uint8_t) 8) & (uint16_t) 0xFF00) |
      (uint16_t) (data[1] & (uint8_t) 0xFF));
  return a;
}

static char get_resp(char resp) {
  if (resp != SOCKS5_CMD_CONNECT) {
    return SOCKS5_REPLY_COMMAND_NOT_SUPPORTED;
  }
  return SOCKS5_REPLY_SUCCEEDED;
}

/**
 * check connect
 */
static void on_connected_write(DocketEvent *event, void *ctx) {
  LOGD("");
  ServerContext *context = ctx;
  DocketBuffer *out = DocketEvent_get_out_buffer(event);
  if (DocketBuffer_length(out) == 0) {
    // transfer data between context and remote
    RemoteContext *remote = context->remote;
    DocketEvent_set_cb(event, docket_server_read, NULL, docket_server_disconnect_event, ctx);

    devent_set_read_enable(remote->event, true);
    DocketEvent_set_cb(remote->event, docket_remote_read, NULL, docket_remote_disconnect_event, remote);
  }
}

#ifndef sprintf_s
#define sprintf_s(c, l, f, ...) sprintf(c, f, __VA_ARGS__)
#endif

static void hexDump(char *buf, int len, int addr) {
  int i, j, k;
  char binstr[80];

  for (i = 0; i < len; i++) {
    if (0 == (i % 16)) {
      sprintf_s(binstr, 80, "%08x -", i + addr);
      sprintf_s(binstr, 80, "%s %02x", binstr, (unsigned char) buf[i]);
    } else if (15 == (i % 16)) {
      sprintf_s(binstr, 80, "%s %02x", binstr, (unsigned char) buf[i]);
      sprintf_s(binstr, 80, "%s  ", binstr);
      for (j = i - 15; j <= i; j++) {
        sprintf_s(binstr, 80, "%s%c", binstr, ('!' < buf[j] && buf[j] <= '~') ? buf[j] : '.');
      }
      printf("%s\n", binstr);
    } else {
      sprintf_s(binstr, 80, "%s %02x", binstr, (unsigned char) buf[i]);
    }
  }
  if (0 != (i % 16)) {
    k = 16 - (i % 16);
    for (j = 0; j < k; j++) {
      sprintf_s(binstr, 80, "%s   ", binstr);
    }
    sprintf_s(binstr, 80, "%s  ", binstr);
    k = 16 - k;
    for (j = i - k; j < i; j++) {
      sprintf_s(binstr, 80, "%s%c", binstr, ('!' < buf[j] && buf[j] <= '~') ? buf[j] : '.');
    }
    printf("%s\n", binstr);
  }
}

#ifdef WIN32
static char* strndup(char *src, unsigned int len) {
  char *str = malloc(len + 1);
  memcpy(str, src, len);
  str[len] = '\0';
  return str;
}

#endif

static void send_client_auth_response(DocketEvent *event, RemoteContext *remote) {
  ServerContext *server = remote->server;

  struct sockaddr_storage bind_address;
  socklen_t bind_socklen = sizeof(bind_address);
  unsigned short port = 0;

  char *resp_data = server->reply_data;
  size_t reply_data_len;

  if (DocketEvent_fetch_local_address(event, (struct sockaddr *) &bind_address, &bind_socklen, &port)) {
    if (bind_address.ss_family == AF_INET) {
      memcpy(resp_data + SOCKS5_CONN_HEADER_LEN, &((struct sockaddr_in *) &bind_address)->sin_addr, IPV4_LEN);
      resp_data[3] = SOCKS5_ATYPE_IPV4;
      reply_data_len = SOCKS5_CONN_HEADER_LEN + IPV4_LEN + PORT_LEN;
    } else if (bind_address.ss_family == AF_INET6) {
      memcpy(resp_data + SOCKS5_CONN_HEADER_LEN, &((struct sockaddr_in6 *) &bind_address)->sin6_addr, IPV6_LEN);
      resp_data[3] = SOCKS5_ATYPE_IPV6;
      reply_data_len = SOCKS5_CONN_HEADER_LEN + IPV6_LEN + PORT_LEN;
    } else {
      LOGW("no way");
      return;
    }
  } else {
    DocketEvent_free(event);
    RemoteContext_free(remote);

    DocketEvent_free(server->event);
    ServerContext_free(server);
    return;
  }

  // bind on local
  LOGD("bind address: %s", sockaddr_to_string((struct sockaddr *) &bind_address, NULL, 0));

  char *bind_port = resp_data + reply_data_len - PORT_LEN;
  // update port
  n_write_u_short_to_data(bind_port, port, 0);
  hexDump(resp_data, (int) reply_data_len, 0);

  DocketEvent_set_write_cb(server->event, on_connected_write, server);
  DocketEvent_write(server->event, resp_data, reply_data_len);
}

static void server_event(DocketEvent *event, int what, void *ctx) {
  if (DEVENT_IS_ERROR_OR_EOF(what)) {
    ServerContext *server = ctx;
    RemoteContext *remote = server->remote;

    server->event = NULL;
    DocketEvent_free(event);
    ServerContext_free(server);

    remote->server = NULL;
    DocketEvent_free(remote->event);
    RemoteContext_free(remote);
  }
}

void docks_on_remote_connect(DocketEvent *event, RemoteContext *remote_context) {
  devent_set_read_enable(event, false);
  send_client_auth_response(event, remote_context);
}

static void on_remote_connect(DocketEvent *event, int what, void *ctx) {
  if (what & DEVENT_CONNECT && !DEVENT_IS_ERROR_OR_EOF(what)) {
    docks_on_remote_connect(event, ctx);
    return;
  }
  RemoteContext *remote = ctx;

  LOGI("connect failed");
  ServerContext *server = remote->server;

  DocketEvent_free(event);
  RemoteContext_free(ctx);

  DocketEvent_free(server->event);
  ServerContext_free(server);
}

static void read_address(DocketEvent *event, void *ctx) {
  ServerContext *context = ctx;

  char req_address[MAX_DOMAIN_LEN];
  int req_address_len;
  char port[2];
  size_t len = DocketBuffer_length(DocketEvent_get_in_buffer(event));

  char *header = context->connect_header;
  char atype = header[3];

  if (atype == SOCKS5_ATYPE_IPV4) {
    if (len < (IPV4_LEN + PORT_LEN)) {
      LOGI("wait more data for Requests");
      return;
    }
    char ip[IPV4_LEN];
    DocketEvent_read_full(event, ip, IPV4_LEN);

    struct sockaddr_in address_in;
    memcpy(&address_in.sin_addr, ip, IPV4_LEN);
    char *ip_str = inet_ntoa(address_in.sin_addr);
    if (ip_str == NULL) {
      LOGI("parse ipv4 failed");
      ServerContext_free(context);
      DocketEvent_free(event);
      return;
    }

    context->remote_host = strdup(ip_str);

    char port_data[PORT_LEN];
    DocketEvent_read_full(event, port_data, PORT_LEN);

    context->remote_port = port_to_uint16(port_data);
    req_address_len = IPV4_LEN;
    memcpy(req_address, ip, IPV4_LEN);
    memcpy(port, port_data, PORT_LEN);

    LOGI("request remote address ipv4: host = %s, port = %d", context->remote_host, context->remote_port);
  } else if (atype == SOCKS5_ATYPE_IPV6) {
    if (len < (IPV6_LEN + PORT_LEN)) {
      LOGI("wait more data for Requests");
      return;
    }
    char ip[IPV6_LEN];
    DocketEvent_read_full(event, ip, IPV6_LEN);

    char ipv6[1024];
    if (inet_ntop(AF_INET6, ip, ipv6, sizeof(ipv6))) {
      context->remote_host = strdup(ipv6);
    } else {
      LOGI("parse ipv6 failed");
      ServerContext_free(context);
      DocketEvent_free(event);
      return;
    }

    char port_data[PORT_LEN];
    DocketEvent_read_full(event, port_data, PORT_LEN);
    context->remote_port = port_to_uint16(port_data);

    req_address_len = IPV6_LEN;
    memcpy(req_address, ip, req_address_len);
    memcpy(port, port_data, PORT_LEN);

    LOGI("request remote address ipv6: host = %s, port = %d", context->remote_host, context->remote_port);
  } else {
    char buffer[MAX_DOMAIN_LEN * 2 + 4];
    ssize_t buffer_len = DocketEvent_read(event, buffer, sizeof(buffer));
    if (buffer_len <= 0) {
      LOGI("wait more data for domain");
      return;
    }

    int domain_len = (unsigned char) buffer[0];
    if (domain_len <= 0 || domain_len > MAX_DOMAIN_LEN) {
      LOGE("bad domain len: %d", domain_len);
      ServerContext_free(context);
      DocketEvent_free(event);
      return;
    }

    if (len < 1 + domain_len + PORT_LEN) {
      LOGI("wait more data for domain");
      return;
    }

    context->remote_host = strndup(buffer + 1, domain_len);
    memcpy(port, buffer + 1 + domain_len, PORT_LEN);
    LOGD("port[0] = %x, [1] = %x", port[0], ((u_short) port[1]) & (u_char) 0xFF);
    context->remote_port = port_to_uint16(port);

    req_address_len = 1 + domain_len;
    req_address[0] = (char) domain_len;
    memcpy(req_address + 1, context->remote_host, domain_len);

    LOGI("request remote address: domain = %s, port = %d", context->remote_host, context->remote_port);
  }

  char *resp_data = context->reply_data;
  resp_data[0] = SOCKS5_VERSION;
  resp_data[1] = get_resp(context->connect_header[1]);
  resp_data[2] = 0x00;

  if (resp_data[1] == SOCKS5_REPLY_SUCCEEDED) {
    // custom connect
    if (context->connect_remote) {
      context->connect_remote(context);
      return;
    }

    RemoteContext *remote = calloc(1, sizeof(RemoteContext));
    remote->server = context;
    remote->event = DocketEvent_connect_hostname(DocketEvent_getDocket(event), -1, context->remote_host,
                                                 context->remote_port);
    if (remote->event == NULL) {
      LOGI("connect to remote failed: %s:[%d]", context->remote_host, context->remote_port);
      ServerContext_free(context);
      DocketEvent_free(event);
      return;
    }
    context->remote = remote;

    DocketEvent_set_event_cb(remote->event, on_remote_connect, remote);
    DocketEvent_set_event_cb(context->event, server_event, context);
    // TODO timer
  } else {
    // TODO close
//        DocketEvent_write(event, resp_data, context->reply_data_len);
//        context->status = SOCKS_STATUS_WAIT_DISCONNECT;
//        DocketEvent_set_read_cb(event, NULL, ctx);
//        DocketEvent_set_write_cb(event, disconnect_write, ctx);
  }
}

void docks_connect_read(DocketEvent *event, void *ctx) {
  ServerContext *context = ctx;

  // read header
  char *header = context->connect_header;

  ssize_t len = DocketEvent_read_full(event, header, SOCKS5_CONN_HEADER_LEN);
  if (len < SOCKS5_CONN_HEADER_LEN) {
    LOGD("len = %d, need read more data", len);
    return;
  }

  if (header[0] != SOCKS5_VERSION) {
    ServerContext_free(context);
    DocketEvent_free(event);
    return;
  }

  if (header[1] != SOCKS5_CMD_CONNECT) {
    LOGI("command(%d) not support", header[1]);
  }

  if (header[2] != 0x00) {
    LOGI("bad reserved(%d)", header[2]);
    ServerContext_free(context);
    DocketEvent_free(event);
    return;
  }

  char atype = header[3];
  if (atype != SOCKS5_ATYPE_IPV4 && atype != SOCKS5_ATYPE_DOMAINNAME && atype != SOCKS5_ATYPE_IPV6) {
    LOGI("unsupport address type");
    ServerContext_free(context);
    DocketEvent_free(event);
    return;
  }

  read_address(event, ctx);
  DocketEvent_set_read_cb(event, read_address, ctx);
}

// client
static void on_read_server_bind_port(DocketEvent *event, void *ctx) {
  ClientContext *client_context = ctx;

  char port[2];
  if (DocketEvent_read_full(event, port, 2) != 2) {
    LOGD("need read more data");
    return;
  }
  LOGD("%d", ((ClientContext *) ctx)->bind_port);
  client_context->bind_port = port_to_uint16(port);
  LOGD("%X", ((ClientContext *) ctx)->on_server_connect);

  LOGD("server bind address: %s:[%d]", client_context->bind_host, client_context->bind_port);

  // on client connect
  if (client_context->on_server_connect) {
    client_context->on_server_connect(event, client_context);
  } else {
    LOGE("client_context->on_server_connect is NULL");
    DocketEvent_free(event);
    ClientContext_free(ctx);
  }
}

static void on_read_server_bind_host(DocketEvent *event, void *ctx) {
  LOGD("%X", ((ClientContext *) ctx)->on_server_connect);
  ClientContext *client_context = ctx;
  char atype[1];
  DocketBuffer *in = DocketEvent_get_in_buffer(event);
  if (DocketBuffer_peek_full(in, atype, 1) != 1) {
    LOGD("need read more data");
    return;
  }
  LOGD("%s", ((ClientContext *) ctx)->bind_host);

  if (atype[0] == SOCKS5_ATYPE_IPV4) {
    size_t data_len = 1 + IPV4_LEN;
    char *data = malloc(data_len);
    if (DocketEvent_read_full(event, data, data_len) != data_len) {
      LOGD("need read more data");
      free(data);
      return;
    }

    struct sockaddr_in address_in;
    memcpy(&address_in.sin_addr, data + 1, IPV4_LEN);
    free(data);
    char *ip_str = inet_ntoa(address_in.sin_addr);
    if (ip_str == NULL) {
      DocketEvent_free(event);
      ClientContext_free(ctx);
      return;
    }
    client_context->bind_host = strdup(ip_str);
  } else if (atype[0] == SOCKS5_ATYPE_IPV6) {
    size_t data_len = 1 + IPV6_LEN;
    char data[1 + IPV6_LEN];

    if (DocketEvent_read_full(event, data, data_len) != data_len) {
      LOGD("need read more data");
      return;
    }

    char ipv6[1024];
    if (inet_ntop(AF_INET6, data + 1, ipv6, sizeof(ipv6))) {
      client_context->bind_host = strdup(ipv6);
    } else {
      DocketEvent_free(event);
      ClientContext_free(ctx);
      return;
    }
  } else if (atype[0] == SOCKS5_ATYPE_DOMAINNAME) {
    char header[2];
    if (DocketBuffer_peek_full(in, header, 2) != 2) {
      LOGD("need read more data");
      return;
    }

    char *domain = malloc(2 + header[1]);
    if (DocketEvent_read_full(event, domain, sizeof(domain)) != sizeof(domain)) {
      LOGD("need read more data");
      free(domain);
      return;
    }
    client_context->bind_host = strndup(domain + 2, header[1]);
    free(domain);
  } else {
    DocketEvent_free(event);
    ClientContext_free(ctx);
    return;
  }

  DocketEvent_set_read_cb(event, on_read_server_bind_port, ctx);
  on_read_server_bind_port(event, ctx);
}

#define CONNECT_RESP_HEADER_LENGTH 3

static void on_read_server_connect(DocketEvent *event, void *ctx) {
  char header[CONNECT_RESP_HEADER_LENGTH];
  if (DocketEvent_read_full(event, header, CONNECT_RESP_HEADER_LENGTH) != CONNECT_RESP_HEADER_LENGTH) {
    LOGD("need read more data");
    return;
  }
  LOGD("%X", ((ClientContext *) ctx)->on_server_connect);

  if (header[0] != SOCKS5_VERSION || header[2] != 0x00) {
    DocketEvent_free(event);
    ClientContext_free(ctx);
    return;
  }

  if (header[1] != SOCKS5_REPLY_SUCCEEDED) {
    LOGE("connect failed: %d", header[1]);
    DocketEvent_free(event);
    ClientContext_free(ctx);
    return;
  }

  DocketEvent_set_read_cb(event, on_read_server_bind_host, ctx);
  on_read_server_bind_host(event, ctx);
}

void docks_send_remote_address(DocketEvent *event, ClientContext *context) {
  char header[] = {SOCKS5_VERSION, SOCKS5_CMD_CONNECT, 0x00, SOCKS5_ATYPE_DOMAINNAME};
  DocketEvent_write(event, header, sizeof(header));
  LOGD("send remote address to socks server: %s %d", context->remote_host, context->remote_port);

  // write host
  char host_len[1] = {(char) strlen(context->remote_host)};
  DocketEvent_write(event, host_len, sizeof(host_len));
  DocketEvent_write(event, context->remote_host, host_len[0]);

  // write port
  char port[2];
  n_write_u_short_to_data(port, context->remote_port, 0);
  DocketEvent_write(event, port, sizeof(port));
  LOGD("%X", context->on_server_connect);

  DocketEvent_set_read_cb(event, on_read_server_connect, context);
}