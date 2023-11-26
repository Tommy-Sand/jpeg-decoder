#include "data.h"

uint8_t read_bit(Data *d) {
	uint8_t *ptr = *(d->encoded_data);
	uint8_t bit = (*ptr >> (7 - (d->offset++))) & 0x1;
	if (d->offset >= 8) {
		next_byte(d);
		d->offset = 0; 
	}
	return bit;
}

uint8_t read_byte(Data *d) {
	if (offset == 0) {
		uint8_t byte = **(d->encoded_data);
		(*(d->encoded_data))++;
	}


}

uint8_t next_byte(Data *d) {
	uint8_t n_byte = *(++(*(d->encoded_data)));
	if (n_byte == 0xFF) {
		uint8_t nn_byte = *((*(d->encoded_data)) + 1);
		if (nn_byte == 0x00) {
			
		}
		else if (nn_byte >= 0xD0 && nn_byte <= 0xD7) {
			
		}
	}
	++(*ptr);
	return 0;
}
