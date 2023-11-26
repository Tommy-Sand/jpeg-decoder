#include "JPEG_decoder.hh"

Restart_interval::Restart_interval(uint8_t **data) {
	uint16_t length = (*((*data)++)) << 8;
	length += *((*data)++);
    this->num_MCUs = *((*data)++) << 8;
    this->num_MCUs += *((*data)++);
}
