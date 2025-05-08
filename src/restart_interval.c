#include "restart_interval.h"

#include <stddef.h>
#include <stdio.h>

int32_t decode_restart_interval(uint8_t **encoded_data, RestartInterval *ri) {
    if (ri == NULL) {
        return -1;
    }
    uint8_t *ptr = *encoded_data;
    uint16_t len = (*(ptr++)) << 8;
    len += (*(ptr++));
    if (len != 4) {
        return -1;
    }

    *ri = (*(ptr++)) << 8;
    *ri += (*(ptr++));
    *encoded_data = ptr;
    return 0;
}
