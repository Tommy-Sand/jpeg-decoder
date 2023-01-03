#include "JPEG_decoder.hh"

uint8_t zigzag[64] = {0x00, 0x10, 0x01, 0x02, 0x11, 0x20, 0x30, 0x21,
					  0x12, 0x03, 0x04, 0x13, 0x22, 0x31, 0x40, 0x50,
					  0x41, 0x32, 0x23, 0x14, 0x05, 0x06, 0x15, 0x24,
					  0x33, 0x42, 0x51, 0x60, 0x70, 0x61, 0x52, 0x43,
					  0x34, 0x25, 0x16, 0x07, 0x17, 0x26, 0x35, 0x44,
					  0x53, 0x62, 0x71, 0x72, 0x63, 0x54, 0x45, 0x36,
					  0x27, 0x37, 0x46, 0x55, 0x64, 0x73, 0x74, 0x65,
					  0x56, 0x47, 0x57, 0x66, 0x75, 0x76, 0x67, 0x77};

Quantization_table::Quantization_table(uint8_t **data) {
    this->percision = ((*(++(*data)) >> 4) * 8) + 8;
    this->id = **data & 0xF;

    this->quant_table = new uint16_t*[8];
    for(int i = 0; i < 8; i++)
        this->quant_table[i] = new uint16_t[8];

    for(int i = 0; i < 64; i++){
        uint8_t vert = zigzag[i] & 0xF;
        uint8_t horz = (zigzag[i] >> 4) & 0xF;
        this->quant_table[vert][horz] = *(++(*data));
        if(percision == 16)
            this->quant_table[vert][horz] = (this->quant_table[vert][horz] << 8) + *(++(*data));
    } 
	this->length = 65 + 64 * (this->percision/16);
}
