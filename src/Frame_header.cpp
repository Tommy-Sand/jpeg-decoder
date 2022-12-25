#include "jpeg_decoder.hh"

enum uint8_t Encoding_process {
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
}