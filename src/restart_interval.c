#include "restart_interval.h"

#include <stddef.h>
#include <stdio.h>

int32_t decode_restart_interval(Bitstream *bs, RestartInterval *ri) {
    if (ri == NULL) {
        return -1;
    }
    //uint8_t *ptr = *encoded_data;
    //uint16_t len = (*(ptr++)) << 8;
    //len += (*(ptr++));
    uint16_t len = next_byte(bs) << 8;
    len += next_byte(bs);
    if (len != 4) {
        return -1;
    }

    //*ri = (*(ptr++)) << 8;
    //*ri += (*(ptr++));
    *ri = next_byte(bs) << 8;
    *ri += next_byte(bs);
    //*encoded_data = ptr;
    return 0;
}
