#include "JPEG_decoder.hh"

Scan_header::Scan_header(uint8_t *data): data{data} {
    uint16_t pos = 1;

    this->length = ((uint16_t) *(data + (++pos))) << 8;
    this->length += *(data + (++pos));

    this->num_chans = *(data + (++pos));

    this->chan_specifiers = new struct Chan_specifier[this->num_chans];
    for(int i = 0; i < this->num_chans; i++){
        this->chan_specifiers[i].componentID = *(data + (++pos));
        this->chan_specifiers[i].Huffman_DC = *(data + (++pos)) >> 4;
        this->chan_specifiers[i].Huffman_AC = *(data + pos) & 0xF;
    }

    this->spectral_start = *(data + (++pos));
    this->spectral_end = *(data + (++pos));

    this->prev_approx = *(data + (++pos)) >> 4;
    this->succ_approx = *(data + pos) & 0xF;
}