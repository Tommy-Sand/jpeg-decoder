#include "jpeg_decoder.hh"

enum Encoding: uint8_t {
    BaseSeq_huff_DCT = 0x0,
    ExtendedSeq_huff_DCT = 0x1,
    Prog_huff_DCT = 0x2,
    Lossless_huff = 0x3,
    Seq_huff_diff_DCT = 0x5,
    Prog_huff_diff_DCT = 0x6,
    Lossless_huff_diff = 0x7,
    Seq_arith_DCT = 0x9,
    Prog_arith_DCT = 0xA,
    Lossless_arith = 0xB,
    Seq_arith_diff_DCT = 0xD,
    Prog_arith_diff_DCT = 0xE,
    Lossless_arith_diff = 0xF,

};

Frame_header::Frame_header(uint8_t *data): data{data} {
    uint16_t pos = 0;
    uint8_t marker_encoding_num = *(data + (++pos)) & 0xF;
    this->encoding_process = static_cast<Encoding>(marker_encoding_num);

    this->length = (uint16_t) *(data + (++pos)) << 8;
    this->length += *(data + (++pos));

    this->precision = *(data + (++pos));

    this->height = (uint16_t) (*(data + (++pos)) << 8);
    this->height = *(data + (++pos));

    this->width = (uint16_t) (*(data + (++pos)) << 8);
    this->width = *(data + (++pos));

    this->num_chans = *(data + (++pos));

    this->chan_infos = new struct Channel_info[num_chans];
    for(int i = 0; i < this->num_chans; i++){
        this->chan_infos[i].id = *(data + (++pos));

        this->chan_infos[i].horz_sampling = (*(data + (++pos)) >> 4) & 0xF;
        this->chan_infos[i].vert_sampling = *(data + pos) & 0xF;

        this->chan_infos[i].qtableID = *(data + (++pos));
    }
}