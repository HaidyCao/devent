//
// Created by Haidy on 2021/11/21.
//

#include "buffer_test.h"
#include "buffer.h"
#include <string.h>

void DocketBuffer_moveto_test() {
    DocketBuffer *in = DocketBuffer_new();
    
    char *data = "data";
    DocketBuffer_write(in, data, strlen(data));

    DocketBuffer *out = DocketBuffer_new();
    DocketBuffer_moveto(out, in);
}