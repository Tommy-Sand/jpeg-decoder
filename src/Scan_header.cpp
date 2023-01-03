#include "JPEG_decoder.hh"

Scan_header::Scan_header(uint8_t **data) {	
    this->length = **data << 8;
    this->length += *(++(*data));

    this->num_chans = *(++(*data));

    this->chan_specifiers = new struct Chan_specifier[this->num_chans];
    for(int i = 0; i < this->num_chans; i++){
        this->chan_specifiers[i].componentID = *(++(*data));
        this->chan_specifiers[i].Huffman_DC = (*(++(*data)) >> 4) & 0xF;
        this->chan_specifiers[i].Huffman_AC = (**data) & 0xF;
    }

    this->spectral_start = *(++(*data));
    this->spectral_end = *(++(*data));

    this->prev_approx = (*(++(*data)) >> 4) & 0xF;
    this->succ_approx = *(++(*data)) & 0xF;
}
