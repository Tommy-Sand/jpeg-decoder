#include <stdint.h>
#include "bitstream.h"

typedef uint16_t RestartInterval;

//int32_t decode_restart_interval(uint8_t **encoded_data, RestartInterval *ri);
int32_t decode_restart_interval(Bitstream *bs, RestartInterval *ri);
