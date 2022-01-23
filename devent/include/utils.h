//
// Created by Haidy on 2021/10/31.
//

#ifndef DEVENT_UTILS_H
#define DEVENT_UTILS_H

#include <stdbool.h>

/**
 * get current error info
 * @return error message included errno and errmsg
 */
char *devent_errno();

bool devent_parse_ipv4(const char *ip, unsigned char *result);

#endif //DEVENT_UTILS_H
