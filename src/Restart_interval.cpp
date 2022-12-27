#include "jpeg_decoder.hh"

Restart_interval::Restart_interval(uint8_t *data): data{data} {
    uint8_t pos = 1;

    this->num_MCUs = *(data + (++pos)) << 8;
    this->num_MCUs += *(data + (++pos));
}