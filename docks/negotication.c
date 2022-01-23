//
// Created by Haidy on 2021/11/7.
//

#include <stddef.h>
#include "negotication.h"
#include "docks_def.h"

char docks_negotication(ServerContext *context, const char *method, unsigned char method_count) {
    ServerConfig *config = context->config;
    if (config->users == NULL || c_hash_map_get_count(config->users) == 0) {
        // no need auth
        return SOCKS5_METHOD_NO_AUTHENTICATION_REQUIRED;
    }

    // check if contains SOCKS5_METHOD_USERNAME_PASSWORD
    int i;
    for (i = 0; i < method_count; i++) {
        if ((u_char) method[i] == SOCKS5_METHOD_USERNAME_PASSWORD) {
            return SOCKS5_METHOD_USERNAME_PASSWORD;
        }
    }
    return SOCKS5_METHOD_NO_ACCEPTABLE_METHODS;
}