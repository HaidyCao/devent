//
// Created by Haidy on 2021/11/21.
//

#ifndef DOCKET_DEF_H
#define DOCKET_DEF_H

typedef struct server_context ServerContext;
typedef struct server_config ServerConfig;
typedef struct remote_context RemoteContext;
typedef struct client_context ClientContext;

#define STR_IS_EMPTY(str) ((str) == NULL || strlen(str) == 0)

#endif //DOCKET_DEF_H
