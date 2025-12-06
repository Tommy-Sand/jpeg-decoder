#include "bitstream.h"
#include "debug.h"

uint8_t get_byte(Bitstream *bs) {
    return *(bs->encoded_data);
}

uint8_t next_byte(Bitstream *bs) {
    uint8_t ret = *(bs->encoded_data);
    bs->size++;
    bs->encoded_data++;
    bs->offset = 0;
    return ret;
}

/**
 * ret: 0 next byte
 *      1 restart marker
 *      2 byte stuffing
 */
// TODO Handle possibility of multiple stuffed bytes in a row
int next_byte_for_bits(Bitstream *bs) {
    uint8_t byte = *(bs->encoded_data);
    if (byte == 0xFF) {
        uint8_t n_byte = *(bs->encoded_data + 1);
        if (n_byte == 0x00) {
            // Byte is stuffed so we skip the 0x00 
            // after 0xFF
            bs->encoded_data = bs->encoded_data + 2;
            bs->offset = 0;
            return 2;
        }
    } else {
        uint8_t n_byte = *(bs->encoded_data + 1);
        if (n_byte == 0xFF) {
            uint8_t nn_byte = *(bs->encoded_data + 2);
            /*
			if (nn_byte >= 0xD0 && nn_byte <= 0xD7) {
				*ptr += 3;
				*offset = 0;
				return 1;
			}
			else if (nn_byte == 0x00) {
			*/
            if (nn_byte == 0x00) {
                bs->encoded_data++;
                bs->offset = 0;
                return 0;
            } else {
                debug_print("New marker detected %X%X\n", n_byte, nn_byte);
                //return -1;
            }
            // TODO handle DNL
        }
    }
    bs->encoded_data++;
    bs->offset = 0;
    return 0;
}

uint8_t next_bit(Bitstream *bs) {
    uint8_t bit = (*bs->encoded_data >> (7 - ((bs->offset)++))) & 0x1;
    if (bs->offset >= 8) {
        next_byte_for_bits(bs);
    }
    return bit;
}

int16_t next_bit_size(Bitstream *bs, uint8_t size) {
    int16_t amp = 0;
    for (int i = 0; i < size; i++) {
        amp = (amp << 1) + ((*bs->encoded_data >> (7 - ((bs->offset)++))) & 0x1);
        if (bs->offset >= 8) {
            next_byte_for_bits(bs);
        }
    }
    return amp;
}

/*
	1 is restart marker
	2 end of image
*/
uint8_t check_marker(Bitstream *bs) {
    uint8_t byte = *bs->encoded_data;
    uint8_t n_byte = *(bs->encoded_data + 1);
    if (byte == 0xFF && n_byte == 0xD9) {
        //End of Image
        debug_print("position of end of image 1\n");
        return 2;
    } else if (byte == 0xFF && (n_byte >= 0xD0 && n_byte <= 0xD7)) {
        //Restart Interval
        debug_print("position of restart interval 1\n");
        return 1;
    } else if (byte == 0xFF && n_byte != 0x00) {
        //Another Marker
        debug_print("position of another marker 1\n");
        return 2;
    }

    uint8_t nn_byte = *(bs->encoded_data + 2);
    if (byte == 0xFF) {
        if (n_byte >= 0xD0 && n_byte <= 0xD7) {
            //Restart Interval
            debug_print("position of restart interval 1\n");
            return 1;
        }

        uint8_t nnn_byte = *(bs->encoded_data + 3);
        if (n_byte == 0x00 && nn_byte == 0xFF
            && (nnn_byte >= 0xD0 && nnn_byte <= 0xD7)) {
            //Restart Interval
            debug_print("position of restart interval 2 %X%X\n", nn_byte, nnn_byte);
            return 1;
        } else if (n_byte == 0x00 && nn_byte == 0xFF && nnn_byte >= 0xD9) {
            //End Of Image
            debug_print("position of end of image 2 %X%X\n", nn_byte, nnn_byte);
            return 2;
        } else if (n_byte == 0x00 && nn_byte == 0xFF) {
            //Another Marker
            debug_print("position of another marker 2 %X%X\n", n_byte, nn_byte);
            return 2;
        }
    } else if (n_byte == 0xFF && (nn_byte >= 0xD0 && nn_byte <= 0xD7)) {
        //Restart Interval
        debug_print("position of restart interval 3 %X%X\n", n_byte, nn_byte);
        return 1;
    } else if (n_byte == 0xFF && nn_byte == 0xD9) {
        //End Of Image
        debug_print("position of end of image 3 %X%X\n", n_byte, nn_byte);
        return 2;
    //TODO Why is commenting this out fix example/imgonline-com-ua-progressive8iLah1czu26m.jpg
    //but not example/progress.jpg
    } else if (n_byte == 0xFF && nn_byte != 0x00) {
        //Another Marker
        debug_print("position of another marker 3 %X%X\n", n_byte, nn_byte);
        return 2;
    } else if (n_byte == 0x00 && nn_byte == 0xFF) {
        //Another Marker
        //debug_print("position of another marker 4 %X%X\n", n_byte, nn_byte);
        //return 2;
    }
    return 0;
}

/**
 * ret: 0 next byte
 *      1 restart marker
 *      2 byte stuffing
 */
// TODO Handle possibility of multiple stuffed bytes in a row
inline int next_byte_restart_marker(Bitstream *bs) {
    uint8_t byte = *bs->encoded_data;
    if (byte == 0xFF) {
        uint8_t n_byte = *(bs->encoded_data + 1);
        if (n_byte == 0x00) {
            // Byte is stuffed and we can skip the 00
            bs->encoded_data = bs->encoded_data + 2;
            bs->offset = 0;
            return 2;
        }
        if (n_byte >= 0xD0 && n_byte <= 0xD7) {
            bs->encoded_data += 2;
            bs->offset = 0;
            return 1;
        }
    } else {
        uint8_t n_byte = *(bs->encoded_data + 1);
        if (n_byte == 0xFF) {
            uint8_t nn_byte = *(bs->encoded_data + 2);
            if (nn_byte >= 0xD0 && nn_byte <= 0xD7) {
                bs->encoded_data += 3;
                bs->offset = 0;
                return 1;
            } else if (nn_byte == 0x00) {
                //*ptr = *ptr + 1;
                //*offset = 0;
                //return 0;
            }
            // TODO handle DNL
        }
    }
    (bs->encoded_data)++;
    bs->offset = 0;
    return 0;
}

int progress(Bitstream *bs, int len) {
    //TODO Add check
    bs->encoded_data += len;
    return 0;
}
