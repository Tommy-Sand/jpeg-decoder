#pragma once
#include <stdint.h>

typedef struct  {
    uint8_t *encoded_data;
    uint8_t offset;
    uint32_t size;
} Bitstream;

uint8_t get_byte(Bitstream *bs);

uint8_t next_byte(Bitstream *bs);

uint8_t next_bit(Bitstream *bs);

uint8_t check_marker(Bitstream *bs);

int next_byte_restart_marker(Bitstream *bs);

int progress(Bitstream *bs, int len);

int16_t next_bit_size(Bitstream *bs, uint8_t size);
