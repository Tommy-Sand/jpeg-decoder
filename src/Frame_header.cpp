#include "JPEG_decoder.hh"

enum Encoding: uint8_t {
    BaseSeq_huff_DCT = 0x0,
    ExtendedSeq_huff_DCT = 0x1,
    Prog_huff_DCT = 0x2,
    };

Frame_header::Frame_header(uint8_t **data) {
    uint8_t marker_encoding_num = **data & 0xF;
    this->encoding_process = static_cast<Encoding>(marker_encoding_num);

    this->length = ((uint16_t) *(++(*data))) << 8;
    this->length += *(++(*data));

    this->precision = *(++(*data));

    this->height = ((uint16_t) *(++(*data))) << 8;
    this->height += *(++(*data));

    this->width = ((uint16_t) *(++(*data))) << 8;
    this->width += *(++(*data));

    this->num_chans = *(++(*data));

    this->chan_infos = new struct Channel_info[num_chans];
    for(int i = 0; i < this->num_chans; i++){
        this->chan_infos[i].id = *(++(*data));

        this->chan_infos[i].horz_sampling = (*(++(*data)) >> 4) & 0xF;
        
		this->chan_infos[i].vert_sampling = **data & 0xF;

        this->chan_infos[i].qtableID = *(++(*data));
    }
}

Frame_header::~Frame_header(){
	delete[] this->chan_infos;
}
