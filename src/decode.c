#include "decode.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "bitstream.h"
#include "dct.h"
#include "debug.h"

//Data Unit Length
#define DU 64

uint8_t *data = NULL; 

// An array of blocks with 64 numbers
typedef struct {
    uint8_t ncs;
    int16_t (**blocks)[DU];
    int16_t *width;
    int16_t *height;
} Buffer;

int write_mcu(Image *img, int16_t (**mcu)[DU], FrameHeader *fh, ScanHeader *sh);
int write_data_unit(
    Image *img,
    uint8_t comp,
    uint16_t x_to_mcu,
    int16_t *du,
    uint32_t x,
    uint32_t y,
    bool precision
);
int decode_scan(
    Bitstream *bs,
    Image *img,
    FrameHeader *fh,
    ScanHeader *sh,
    HuffTables *hts,
    QuantTables *qts,
    RestartInterval ri
);
int decode_progressive_scan(
    Bitstream *bs,
    Image *img,
    FrameHeader *fh,
    HuffTables *hts,
    QuantTables *qts,
    RestartInterval ri,
    ScanHeader *sh,
    Buffer *prog_buf
);
int decode_data_unit(
    Bitstream *bs,
    int16_t *du,
    HuffTable ac,
    HuffTable dc,
    int16_t *pred
);
void restart_marker(int16_t *pred, uint8_t len);
int read_app_segment(Bitstream *bs);
void print_image(Image *img);

Buffer *allocate_mcus_progressive(FrameHeader *fh) {
    Buffer *buffer = (Buffer *) malloc(sizeof(Buffer));
    buffer->ncs = fh->ncs;
    buffer->blocks = (int16_t (**)[DU]) malloc(fh->ncs * sizeof(int16_t (**)[DU]));
    buffer->width = (int16_t *) malloc(fh->ncs * sizeof(int16_t));
    buffer->height = (int16_t *) malloc(fh->ncs * sizeof(int16_t));
    for (int i = 0; i < fh->ncs; i++) {
        Component *c = fh->cs + i;

        uint16_t x_blocks = (c->x
            + ((c->x % (8 * c->hsf)) ? (8 * c->hsf - (c->x % (8 * c->hsf))) : 0
            )) / 8;
        uint16_t y_blocks = (c->y
            + ((c->y % (8 * c->vsf)) ? (8 * c->vsf - (c->y % (8 * c->vsf))) : 0
            )) / 8;

        *(buffer->blocks + i) = (int16_t (*)[DU]) calloc(x_blocks * y_blocks * DU, sizeof(int16_t));
        *(buffer->width + i) = x_blocks;
        *(buffer->height + i) = y_blocks;
        debug_print("width %d\n", x_blocks);
        debug_print("height %d\n", y_blocks);

        debug_print("mcus_progressive size %d\n", x_blocks * y_blocks * DU);
    }

    return buffer;
}

Image *allocate_img(FrameHeader *fh) {
    Image *img = calloc(1, sizeof(Image));
    if (!img) {
        return NULL;
    }
    img->buf = (uint8_t **) calloc(fh->ncs, sizeof(uint8_t *));
    img->width = (int *) calloc(fh->ncs, sizeof(int *));
    if (!img->buf) {
        free(img);
        return NULL;
    }
    for (int i = 0; i < fh->ncs; i++) {
        Component *c = fh->cs + i;
        uint16_t x_to_mcu = c->x
            + ((c->x % (8 * c->hsf)) ? (8 * c->hsf - (c->x % (8 * c->hsf))) : 0
            );
        uint16_t y_to_mcu = c->y
            + ((c->y % (8 * c->vsf)) ? (8 * c->vsf - (c->y % (8 * c->vsf))) : 0
            );
        img->buf[i] = (uint8_t *) calloc(x_to_mcu * y_to_mcu, sizeof(uint8_t));
        img->width[i] = x_to_mcu;
        if (!(img->buf[i])) {
            for (uint8_t j = 0; j < i; j++) {
                free(img->buf[j]);
            }
            free(img);
            return NULL;
        }
    }
    img->n_du = fh->ncs;
    img->mcu = 0;

    return img;
}

void free_img(Image *img) {
    for (int i = 0; i < img->n_du; i++) {
        free(img->buf[i]);
    }
    free(img->buf);
    free(img);
}

int decode_jpeg_buffer(uint8_t *buf, size_t len, FrameHeader *fh, Image **img) {
    data = buf;
    //printf("%p\n", data);
    if (fh == NULL) {
        printf("Frame Header provided is NULL\n");
        return -1;
    }

    ScanHeader sh;
    Image *img_ = NULL;
    Buffer *progressive_buffer = NULL;
    bool init_qts = false;
    QuantTables qts;
    bool init_hts = false;
    HuffTables hts;
    RestartInterval ri = 0;

    Bitstream bs = {
        .encoded_data = buf,
        .size = len
    };

    for (; bs.encoded_data  < buf + len;) {
        bool EOI = false;
        if (next_byte(&bs) == 0xFF) {
            switch (next_byte(&bs)) {
                case 0x01:  //Temp private use in arithmetic coding
                //Markers from
                case 0x02:
                //Through
                case 0xBF:
                    //Are Reserved
                    break;

                case 0xC0:  // SOF Baseline Sequential DCT Huffman
                    debug_print("DEBUG: SOF Baseline DCT Huffman\n");
                    if (decode_frame_header(BDCT, &bs, fh) == -1) {
                        debug_print("DEBUG: frame header read failed\n");
                        return -1;
                    }
                    print_frame_header(fh);
                    if (img_ == NULL) {
                        img_ = allocate_img(fh);
                        if (!img_) {
                            debug_print("Cannot allocate image\n");
                            return -1;
                        }
                    }
                    break;
                case 0xC1:  //SOF Extended Sequential DCT Huffman
                    debug_print("DEBUG: SOF Extended Sequential DCT Huffman\n");
                    if (decode_frame_header(ESDCTHC, &bs, fh) == -1) {
                        debug_print("DEBUG: frame header read failed\n");
                        return -1;
                    }
                    print_frame_header(fh);
                    if (img_ == NULL) {
                        img_ = allocate_img(fh);
                        if (!img_) {
                            printf("Cannot allocate image");
                            return -1;
                        }
                    }
                    break;
                case 0xC2:  //SOF Progressive DCT Huffman
                    debug_print("DEBUG: SOF Progressive DCT Huffman\n");
                    if (decode_frame_header(PDCTHC, &bs, fh) == -1) {
                        debug_print("DEBUG: frame header read failed\n");
                        return -1;
                    }
                    print_frame_header(fh);
                    if (progressive_buffer == NULL) {
                        progressive_buffer = allocate_mcus_progressive(fh);
                        if (!progressive_buffer) {
                            debug_print("Cannot allocate progressive buffer\n");
                            return -1;
                        }
                    }
                    if (img_ == NULL) {
                        img_ = allocate_img(fh);
                        if (!img_) {
                            debug_print("Cannot allocate image\n");
                            return -1;
                        }
                    }
                    break;
                case 0xC3:  //SOF Lossless Sequential Huffman
                    debug_print("DEBUG: SOF Lossless DCT Huffman\n");
                    return -1;
                    break;
                case 0xC5:  //SOF Differential sequential DCT Huffman
                    debug_print("DEBUG: SOF Differential Sequential DCT\n");
                    return -1;
                    break;
                case 0xC6:  //SOF Differential progressive DCT Huffman
                    debug_print("DEBUG: SOF Differential Progressive DCT\n");
                    return -1;
                    break;
                case 0xC7:  //SOF Differential lossless (sequential) Huffman
                    debug_print("DEBUG: SOF Differential lossless DCT\n");
                    return -1;
                    break;

                case 0xC8:  //SOF Reserved for JPEG extensions
                    debug_print("DEBUG: SOF Reserved for JPEG extensions\n");
                    return -1;
                    break;
                case 0xC9:  //SOF Extended sequential DCT Arithmetic
                    debug_print("DEBUG: SOF Extended sequential DCT\n");
                    return -1;
                    break;
                case 0xCA:  //SOF Progressive DCT Arithmetic
                    debug_print("DEBUG: SOF Progressive DCT\n");
                    return -1;
                    break;
                case 0xCB:  //SOF Lossless (sequential) Arithmetic
                    debug_print("SOF Lossless (sequential)\n");
                    return -1;
                    break;

                case 0xCD:  //SOF Differential sequential DCT Arithmetic
                    debug_print("DEBUG: SOF Differential sequential DCT\n");
                    return -1;
                    break;
                case 0xCE:  //SOF Differential progressive DCT Arithmetic
                    debug_print("DEBUG: SOF Differential progressive DCT\n");
                    return -1;
                    break;
                case 0xCF:  //SOF Differential lossless (sequential) Arithmetic
                    debug_print("DEBUG: SOF Differential lossless\n");
                    return -1;
                    break;

                case 0xC4:  //Define Huffman table(s)
                    debug_print("DEBUG: Define Huffman Table(s)\n");
                    if (!init_hts) {
                        if (new_huff_tables(fh->process, &hts) == -1) {
                            debug_print("DEBUG: new_huff_tables failed\n");
                            return -1;
                        }
                        init_hts = true;
                    }
                    if (decode_huff_tables(&bs, &hts) != 0) {
                        debug_print("DEBUG: Huff Table read failed\n");
                        return -1;
                    }
                    break;

                case 0xCC:  //Define arithmetic coding conditioning(s)
                    debug_print(
                        "DEBUG: Define Arithmetic coding conditioning(s)\n"
                    );
                    break;

                //For D0-D7 Restart with modulo 8 count “m”
                case 0xD0:
                case 0xD1:
                case 0xD2:
                case 0xD3:
                case 0xD4:
                case 0xD5:
                case 0xD6:
                case 0xD7:
                    debug_print("DEBUG: Restart marker\n");
                    break;
                case 0xD8:  //SOI
                    debug_print("DEBUG: SOI\n");
                    break;
                case 0xD9:  //EOI
                    debug_print("DEBUG: EOI\n");
                    //return 0;
                    EOI = true;
                    break;
                case 0xDA:  //SOS
                    debug_print("DEBUG: SOS\n");
                    //print_huff_tables(&hts);

                    if (decode_scan_header(&bs, &sh) != 0) {
                        debug_print("DEBUG: Scan Header read failed\n");
                        return -1;
                    }

                    print_scan_header(&sh);
                    if (!init_qts) {
                        debug_print(
                            "DEBUG: Could not decode scan no quantization table provided\n"
                        );
                        return -1;
                    }
                    if (fh->process == PDCTHC) {
                        if (decode_progressive_scan(&bs, img_, fh, &hts, &qts, ri, &sh, progressive_buffer) != 0) {
                            debug_print("DEBUG: Decode Scan failed\n");
                            return -1;
                        }
                    } else {
                        if (decode_scan(&bs, img_, fh, &sh, &hts, &qts, ri) != 0) {
                            debug_print("DEBUG: Decode Scan failed\n");
                            return -1;
                        }
                    }
                    break;
                case 0xDB:  //Define Quant Table
                    debug_print("DEBUG: Definition of quant table\n");
                    if (decode_quant_table(&bs, &qts) == -1) {
                        debug_print("DEBUG: Quant Table read failed\n");
                    }
                    init_qts = true;
                    break;
                case 0xDC:  //Define number of lines
                    debug_print("DEBUG: Defined Number of lines\n");
                    if (decode_number_of_lines(&bs, fh) != 0) {
                        debug_print("DEBUG: Number of Lines read fialed\n");
                    }
                    break;
                case 0xDD:  //Define restart interval
                    debug_print("DEBUG: Define restart interval\n");
                    if (decode_restart_interval(&bs, &ri) != 0) {
                        debug_print("DEBUG: Restart Interval read failed\n");
                        return -1;
                    }

                    break;
                case 0xDE:  //Define Hierarchical Progression
                    debug_print("DEBUG: Hierarchical progression\n");
                    break;
                case 0xDF:  //Expand reference components
                    debug_print("DEBUG: Expand reference components\n");
                    break;

                //For E0-EF Reserved for Application segment
                case 0xE0:
                case 0xE1:
                case 0xE2:
                case 0xE3:
                case 0xE4:
                case 0xE5:
                case 0xE6:
                case 0xE7:
                case 0xE8:
                case 0xE9:
                case 0xEA:
                case 0xEB:
                case 0xEC:
                case 0xED:
                case 0xEE:
                case 0xEF:
                    debug_print("DEBUG: APPLICATION Segment\n");
                    read_app_segment(&bs);
                    break;

                //For F0-FD Reserved for JPEG extensions
                case 0xF0:
                case 0xF1:
                case 0xF2:
                case 0xF3:
                case 0xF4:
                case 0xF5:
                case 0xF6:
                case 0xF7:
                case 0xF8:
                case 0xF9:
                case 0xFA:
                case 0xFB:
                case 0xFC:
                case 0xFD:
                    debug_print("DEBUG: Reserved JPEG extensions\n");
                    break;

                case 0xFE:  //Comment
                    debug_print("DEBUG: Comment\n");
                    break;
            }
        }

        if (EOI) {
            break;
        }
    }

    *img = img_;
    if (hts.nDCAC > 0) {
        free_huff_tables(&hts);
    }

    print_image(*img);
    return 0;
}

int read_app_segment(Bitstream *bs) {
    uint16_t len = next_byte(bs) << 8;
    len += next_byte(bs);

    if (progress(bs, len - 2)) {
        return -1;
    }
    return 0;
}

int write_mcu(Image *img, int16_t (**mcu)[DU], FrameHeader *fh, ScanHeader *sh) {
    for (int i = 0; i < sh->nics; i++) {
        ImageComponent ic = sh->ics[i];

        int p = 0;
        Component *c = NULL;
        for (/*uint8_t*/ p = 0; p < fh->ncs; p++) {
            c = (fh->cs) + p;
            if (c->id == ic.sc) {
                break;
            }
        }
        if (c == NULL) {
            return -1;
        }

        uint8_t hsf = c->hsf;
        uint8_t vsf = c->vsf;
        if (sh->nics == 1) {
            hsf = vsf = 1;
        }

        for (int j = 0; j < vsf; j++) {
            for (int k = 0; k < hsf; k++) {
                //Rounded to the nearest multiple of 8 * c->hsf
                uint16_t img_width = img->width[p]; //352
                uint16_t x_to_mcu = c->x //344
                    + ((c->x % (8 * hsf))
                           ? (8 * hsf - (c->x % (8 * hsf)))
                           : 0);

                uint16_t y_to_mcu = (c->y //44
                    + ((c->y % (8 * vsf))
                           ? (8 * vsf - (c->y % (8 * vsf)))
                           : 0));

                //Current mcu that was decoded
                uint16_t mcu_progress = img->mcu;

                uint16_t x = ((mcu_progress * hsf * 8) + (k * 8)) % img_width;
                uint16_t y =
                    ((((mcu_progress * hsf * 8) + (k * 8)) / img_width)
                     * vsf * 8)
                    + (j * 8);
                if (img_width > x_to_mcu) {
                    x = ((mcu_progress * hsf * 8) + (k * 8)) % x_to_mcu;
                    y =
                        ((((mcu_progress * hsf * 8) + (k * 8)) / x_to_mcu)
                         * vsf * 8)
                        + (j * 8);
                }

                int16_t *du = *((*(mcu + i)) + ((j * hsf) + k));

                debug_print("c->x: %d x_to_mcu: %d y_to_mcu: %d x: %d y: %d\n", c->x, x_to_mcu, y_to_mcu, x, y);
                debug_print("hsf: %d vsf: %d mcu_progress: %d\n", hsf, vsf, mcu_progress);
                write_data_unit(
                    img,
                    p,
                    img_width,
                    du,
                    x,
                    y,
                    fh->precision == 12
                );
            }
        }
    }
    img->mcu++;
    return 0;
}

int write_data_unit(
    Image *img,
    uint8_t comp,
    uint16_t x_to_mcu,
    int16_t *du,
    uint32_t x,
    uint32_t y,
    bool precision
) {
    uint8_t *comp_buf = *(img->buf + comp);
    for (uint32_t j = y; j < y + 8; j++) {
        for (uint32_t k = x; k < x + 8; k++) {
            if (precision) {
                /* TODO A precision of 12 bits is scaled down to 8 bits.
				 * This should be changed as we should support 12 bits of
				 * precision in the viewer */
                *(comp_buf + (j * x_to_mcu) + k) =
                    (uint8_t) ((du[((j - y) * 8) + (k - x)] / 4096.0) * 255.0);
            } else {
                *(comp_buf + (j * x_to_mcu) + k) =
                    (uint8_t) du[((j - y) * 8) + (k - x)];
            }
        }
    }
    return 0;
}

int decode_scan(
    Bitstream *bs,
    Image *img,
    FrameHeader *fh,
    ScanHeader *sh,
    HuffTables *hts,
    QuantTables *qts,
    RestartInterval ri
) {
    /**
	 * For each image component in scan header order
	 * find how many 8x8 blocks for an image component
	 * Read those image components and decode
	 */
    int16_t pred[sh->nics];
    for (int i = 0; i < sh->nics; i++) {
        pred[i] = 0;
    }

    uint8_t EOI = 0;
    int16_t(**mcu)[DU] = (int16_t(**)[DU]) calloc(sh->nics, sizeof(int16_t (**)[DU]));
    if (!mcu) {
        return -1;
    }
    for (int i = 0; i < sh->nics; i++) {
        ImageComponent ic = sh->ics[i];

        Component *c = NULL;
        for (int j = 0; j < fh->ncs; j++) {
            c = (fh->cs) + j;
            if (c->id == ic.sc) {
                break;
            }
        }
        if (c == NULL) {
            return -1;
        }
        int16_t(*dus)[DU] =
            (int16_t(*)[DU]) calloc(DU * (c->hsf * c->vsf), sizeof(int16_t));
        *(mcu + i) = dus;
    }

    uint32_t mcus_read = 0;
    while (!EOI) {
        uint8_t ret = check_marker(bs);
        if ((ri != 0) && ((mcus_read % ri) == 0) && ret == 1) {
            if (ret == 1) {
                restart_marker(pred, sh->nics);
                next_byte_restart_marker(bs);
            }
        } else if (ret == 2) {
            debug_print("End Reached");
            EOI = 1;
            break;
        }

        for (int i = 0; i < sh->nics; i++) {
            int16_t(*dus)[DU] = *(mcu + i);
            ImageComponent ic = sh->ics[i];

            Component *c = NULL;
            for (int j = 0; j < fh->ncs; j++) {
                c = (fh->cs) + j;
                if (c->id == ic.sc) {
                    break;
                }
            }
            if (c == NULL) {
                return -1;
            }

            HuffTable dc = hts->DCAC[0][ic.dc];
            HuffTable ac = hts->DCAC[1][ic.ac];
            QuantTable *qt = qts->tables + c->qtid;
            for (int j = 0; j < c->vsf; j++) {
                for (int k = 0; k < c->hsf; k++) {
                    int16_t *du = *(dus + ((j * c->hsf) + k));
                    memset(du, 0, DU * sizeof(int16_t));
                    if (!du) {
                        return -1;
                    }
                    if (decode_data_unit(
                            bs,
                            du,
                            dc,
                            ac,
                            pred + i
                        )
                        != 0) {
                        free(mcu);
                        return -1;
                    }

                    debug_print("Decoded\n");
                    for (uint16_t x = 0; x < 8; x++) {
                        for (uint16_t y = 0; y < 8; y++) {
                            debug_print("%d ", du[(x * 8) + y]);
                        }
                        debug_print("\n");
                    }
                    debug_print("\n");

                    if (dequant_data_unit(qt, du) != 0) {
                        free(mcu);
                        return -1;
                    }
                    debug_print("Dequant\n");
                    for (uint16_t x = 0; x < 8; x++) {
                        for (uint16_t y = 0; y < 8; y++) {
                            debug_print("%d ", du[(x * 8) + y]);
                        }
                        debug_print("\n");
                    }
                    debug_print("\n");

                    fast_2didct(du, fh->precision == 12);
                }
            }
        }
        write_mcu(img, mcu, fh, sh);
        mcus_read++;
    }

    for (uint8_t i = 0; i < sh->nics; i++) {
        free(*(mcu + i));
    }
    free(mcu);
    return 0;
}

int decode_data_unit(
    Bitstream *bs,
    int16_t *du,
    HuffTable dc_huff,
    HuffTable ac_huff,
    int16_t *pred
) {
    int16_t curr_code = next_bit(bs);
    uint8_t mag = 0;
    for (uint8_t i = 0; i < MAX_CODE_LEN; i++) {
        if (dc_huff.max_codes[i] >= curr_code) {
            mag = (uint8_t)
                * (dc_huff.symbols[i] + (curr_code - dc_huff.min_codes[i]));
            break;
        }
        curr_code = (curr_code << 1) + next_bit(bs);
    }
    int16_t dc = next_bit_size(bs, mag);

    if (dc < (1 << (mag - 1))) {
        dc += -1 - ((1 << mag) - 2);
    }

    dc += *pred;
    *pred = dc;
    du[0] = dc;

    for (uint8_t i = 1; i < DU; i++) {
        curr_code = next_bit(bs);
        for (uint8_t j = 0; j < MAX_CODE_LEN; j++) {
            if (ac_huff.max_codes[j] >= curr_code) {
                mag = (uint8_t)
                    * (ac_huff.symbols[j]
                       + (uint8_t) (curr_code - ac_huff.min_codes[j]));
                break;
            }
            curr_code = (curr_code << 1) + next_bit(bs);
        }

        uint8_t size = mag % 16;
        uint8_t run = (mag >> 4) & 0xF;

        i += run;
        if (mag == 0x00) {
            break;
        } else if (mag == 0xF0) {
            continue;
        }

        int16_t amp = next_bit_size(bs, size);

        if (amp < (1 << (size - 1))) {
            amp += -(1 << size) + 1;
        }
        du[i] = amp;
    }

    return 0;
}


///Progressive decoding
//Seperate functions for decoding dc coefficients
//and ac coefficients

int decode_correction(
    Bitstream *bs,
    int16_t *du,
    ScanHeader *sh
);

int decode_progressive_dc(
    Bitstream *bs,
    int16_t *du,
    HuffTable dc_huff,
    ScanHeader *sh,
    int16_t *pred
);

int decode_progressive_dc_refine(
    Bitstream *bs,
    int16_t *du
);

int decode_progressive_ac(
    Bitstream *bs,
    int16_t *du,
    HuffTable ac_huff,
    ScanHeader *sh
);

int decode_progressive_ac_refine(
    Bitstream *bs,
    int16_t *du,
    HuffTable ac_huff,
    ScanHeader *sh
);

enum ScanMode {
    DC,
    AC
};

int decode_progressive_scan(
    Bitstream *bs,
    Image *img,
    FrameHeader *fh,
    HuffTables *hts,
    QuantTables *qts,
    RestartInterval ri,
    ScanHeader *sh,
    Buffer *prog_buf
) {
    /**
	 * For each image component in scan header order
	 * find how many 8x8 blocks for an image component
	 * Read those image components and decode
	 */
    
    /* 
     * In a progressive scan decode if the spectral 
     * start and spectral end is 0 then we are only
     * decoding the DC coefficient. If the spectral
     * start and end are not 0 then it it is 
     * decoding AC coefficients. Spectral start of
     * 0 with spectral end greater than 0 is not 
     * allowed refer to G.1.1.1.1 in the JPEG spec
     */
    enum ScanMode sm = DC;
    if (sh->ss > 0 && sh->se > 0) {
        sm = AC;  
    }

    int16_t pred[sh->nics];
    for (uint8_t i = 0; i < sh->nics; i++) {
        pred[i] = 0;
    }

    uint8_t EOI = 0;
    int16_t(**mcu)[DU] = (int16_t(**)[DU]) calloc(sh->nics, sizeof(int16_t (**)[DU]));
    if (!mcu) {
        return -1;
    }
    //TODO Refactor sh->nics should be guarentted to be 1 when
    //decoding progressive scan of only ac coefficients
    for (uint8_t i = 0; i < sh->nics; i++) {
        ImageComponent ic = sh->ics[i];

        Component *c = NULL;
        int16_t (*blocks)[DU] = NULL;
        for (uint8_t j = 0; j < fh->ncs; j++) {
            c = (fh->cs) + j;
            if (c->id == ic.sc) {
                blocks = *(prog_buf->blocks + j);
                break;
            }
        }
        if (!c || !blocks) {
            return -1;
        }

        int16_t(*dus)[DU];
        if (sh->nics == 1) {
            dus =
                (int16_t(*)[DU]) calloc(DU, sizeof(int16_t));
        } else {
            dus =
                (int16_t(*)[DU]) calloc(DU * c->hsf * c->vsf, sizeof(int16_t));
        }

        if (!dus) {
            return -1;
        }

        *(mcu + i) = dus;
    }

    int mcus_read = 0;
    img->mcu = 0;

    int eob = 0;
    int block_reads[sh->nics];
    for (int i = 0; i < sh->nics; i++) {
        block_reads[i] = 0;
    }

    int max_mcus = -1;
    while (!EOI) {
        uint8_t ret = check_marker(bs);
        if ((ri != 0) && ((mcus_read % ri) == 0)) {
            if (ret == 1) {
                restart_marker(pred, sh->nics);
                next_byte_restart_marker(bs);
            } else {
                debug_print("restart marker expected, but not found\n");
                exit(1);
            }
        } else if (max_mcus != -1 && (mcus_read >= max_mcus) && ret != 0) {
            debug_print("End Reached\n");
            EOI = 1;
            break;
        } else if (mcus_read >= max_mcus) {
            debug_print("Error reading beyond max mcus\n");
            exit(1);
        }
        
        uint8_t i = 0;
        for (; i < sh->nics; i++) {
            ImageComponent ic = sh->ics[i];

            Component *c = NULL;
            int16_t (*blocks)[DU] = NULL;
            uint16_t width = 0;
            uint16_t height = 0;
            for (int j = 0; j < fh->ncs; j++) {
                c = (fh->cs) + j;
                if (c->id == ic.sc) {
                    blocks = *(prog_buf->blocks + j);
                    width = *(prog_buf->width + j);
                    height = *(prog_buf->height + j);
                    break;
                }
            }

            if (c == NULL) {
                return -1;
            }

            uint8_t hsf = c->hsf;
            uint8_t vsf = c->vsf;
            if (sh->nics == 1) {
                hsf = vsf = 1;
            }

            int16_t (*dus)[DU] = *(mcu + i);

            HuffTable dc = hts->DCAC[0][ic.dc];
            HuffTable ac = hts->DCAC[1][ic.ac];
            QuantTable *qt = qts->tables + c->qtid;
            
            for (int j = 0; j < vsf; j++) {
                for (int k = 0; k < hsf; k++) {

                    uint16_t img_width = width;
                    uint16_t x_to_mcu = (c->x
                        + ((c->x % (8 * hsf))
                               ? (8 * hsf - (c->x % (8 * hsf)))
                               : 0)) / 8;
                    uint16_t y_to_mcu = (c->y
                        + ((c->y % (8 * vsf))
                               ? (8 * vsf - (c->y % (8 * vsf)))
                               : 0)) / 8;

                    max_mcus = x_to_mcu * y_to_mcu;

                    uint16_t x = ((mcus_read * hsf) + k) % img_width;
                    uint16_t y = (((mcus_read * hsf + k) / img_width) * vsf) + j;
                    if (img_width > x_to_mcu) {
                        x = ((mcus_read * hsf) + k) % x_to_mcu;
                        y = ((((mcus_read * hsf) + k) / x_to_mcu) * vsf) + j;
                    }

                    int16_t *hd_du = *(blocks + ((y * img_width + x)));

                    int16_t *du = *(dus + ((j * hsf) + k));

                    if (y >= height) {
                        return 0;
                    }

                    debug_print("Previous Data Unit\n");
                    for (int x = 0; x < 8; x++) {
                        for (int y = 0; y < 8; y++) {
                            debug_print("%d ", hd_du[(x * 8) + y]);
                        }
                        debug_print("\n");
                    }

                    if (eob > 0 && sm == AC && sh->ah > 0) {
                        decode_correction(bs, hd_du, sh);
                        eob--;
                    } else if (sm == DC && sh->ah == 0) {
                        if (decode_progressive_dc(
                            bs,
                            hd_du, dc, sh, pred + i
                        ) != 0) {
                            debug_print("progressive_dc failed\n");
                            return -1;
                        }
                    } else if (sm == DC && sh->ah > 0) {
                        if (decode_progressive_dc_refine(
                            bs,
                            hd_du
                        ) != 0) {
                            debug_print("refining dc failed\n");
                            return -1;
                        }
                    } else if (sm == AC && sh->ah == 0) {
                        eob = decode_progressive_ac(bs, hd_du, ac, sh);
                        //TODO ADD RETURN ERROR IN FUNCTION
                        if (eob < 0) {
                            debug_print("eob less than 0 \n");
                            return -1;
                        }
                    } else if (sm == AC && sh->ah > 0) {
                        eob = decode_progressive_ac_refine(bs, hd_du, ac, sh);
                        if (eob < 0) {
                            debug_print("eob less than 0 \n");
                            return -1;
                        }
                    } else {
                        return -1;
                    }

                    memcpy(du, hd_du, DU * sizeof(int16_t));

                    
                    if (dequant_data_unit(qt, du) != 0) {
                        debug_print("failed dequant\n");
                        free(du);
                        return -1;
                    }

                    fast_2didct(du, fh->precision == 12);
                }
            }
            block_reads[i]++;
        }
        write_mcu(img, mcu, fh, sh);
        if (eob && sh->ah == 0) {
            img->mcu += eob;
            mcus_read += eob;
            block_reads[i] += eob;
        }
        mcus_read++;
    }

    for (int i = 0; i < sh->nics; i++) {
        free(*(mcu + i));
    }
    free(mcu);
    return 0;
}

int decode_progressive_dc(
    Bitstream *bs,
    int16_t *du,
    HuffTable dc_huff,
    ScanHeader *sh,
    int16_t *pred
) {
    int16_t curr_code = next_bit(bs);
    uint8_t mag = 0;
    int i = 0;
    for (/*uint8_t*/ i = 0; i < 16; i++) {
        if (dc_huff.symbols[i] == NULL) {
            curr_code = (curr_code << 1) + next_bit(bs);
            continue;
        }

        if (dc_huff.max_codes[i] >= curr_code) {
            mag = (uint8_t)
                *(dc_huff.symbols[i] + (curr_code - dc_huff.min_codes[i]));
            break;
        }
        curr_code = (curr_code << 1) + next_bit(bs);
    }
    int16_t dc = 0;
    for (int j = 0; j < mag; j++) {
        dc = (dc << 1) + next_bit(bs);
    }

    if (dc < (1 << (mag - 1))) {
        dc += -1 - ((1 << mag) - 2);
    }

    dc += *pred;
    *pred = dc;

    dc <<= sh->al;

    du[0] = dc;

    return 0;
}

int decode_progressive_dc_refine(
    Bitstream* bs,
    int16_t *du
) {
    du[0] |= next_bit(bs);

    return 0;
} 

int decode_progressive_ac(
    Bitstream *bs,
    int16_t *du,
    HuffTable ac_huff,
    ScanHeader *sh
) {
    for (int i = sh->ss; i <= sh->se; i++) {
        uint8_t mag = 0;
        int16_t curr_code = next_bit(bs);
        debug_print("huffman: %d\n", curr_code);
        for (int j = 0; j < 16; j++) {
            if (ac_huff.symbols[j] == NULL) {
                curr_code = (curr_code << 1) + next_bit(bs);
                continue;
            }

            if (ac_huff.max_codes[j] >= curr_code) {
                mag = (uint8_t)
                    * (ac_huff.symbols[j]
                       + (uint8_t) (curr_code - ac_huff.min_codes[j]));
                break;
            }
            curr_code = (curr_code << 1)  + next_bit(bs);
        }
        debug_print("\n");

        uint8_t size = mag % 16;
        uint8_t run = (mag >> 4) & 0xF;

        int16_t amp = 0;
        if (size) {
            debug_print("ac_bits: ");
        }
        for (int j = 0; j < size; j++) {
            amp = (amp << 1) + next_bit(bs);
        }
        debug_print("\n");

        if (amp < (1 << (size - 1))) {
            amp += -(1 << size) + 1;
        }

        amp = amp << sh->al;

        uint16_t eob = 1;
        if (run < 0xF && run > 0x0 && size == 0x0) {
            debug_print("eob bits ");
            for (int j = 0; j < run; j++) {
                eob = (eob << 1) + next_bit(bs);
            }
            debug_print("\n");
        }

        if (mag == 0xF0) { //ZRL
            i += (0x10 - 1);
            continue;
        } else {
            i += run;
        }

        if (run < 0xF && size == 0) {
            debug_print("additional end of blocks %d\n", eob - 1);
            return eob - 1;
        }

       
        if (du[i] != 0) {
            debug_print("ERROR OCCURED DU[I] SHOULD BE ZERO\n");
        }
        du[i] = amp;
    }

    return 0;
}

int decode_progressive_ac_refine(
    Bitstream *bs,
    int16_t *du,
    HuffTable ac_huff,
    ScanHeader *sh
) {
    for (int i = sh->ss; i <= sh->se; i++) {
        uint8_t mag = 0;
        int16_t curr_code = next_bit(bs);
        debug_print("huffman %d", curr_code);
        for (int j = 0; j < 16; j++) {
            if (ac_huff.symbols[j] == NULL) {
                curr_code =
                    (curr_code << 1) + next_bit(bs);
                continue;
            }

            if (ac_huff.max_codes[j] >= curr_code) {
                mag = (uint8_t)
                    * (ac_huff.symbols[j]
                       + (uint8_t) (curr_code - ac_huff.min_codes[j]));
                break;
            }
            curr_code =
                (curr_code << 1) + next_bit(bs);
        }
        debug_print("\n");

        uint8_t size = mag % 16;
        uint8_t run = (mag >> 4) & 0xF;
        debug_print("size: %d run : %d\n", size, run);

        if (size) {
            //Read amp
            int16_t amp = next_bit(bs);
            debug_print("ac_bits %d\n", amp);

            if (!amp) {
                amp = -1;
            }
            amp = amp << sh->al;

            //Correct the run numbers or move to the next zero
            int j = 0;
            if (j < run || du[i] != 0) {
                debug_print("refine_bits ");
            }
            while (j < run || du[i] != 0) {
                if (du[i] == 0) {
                    j++;
                } else {
                    uint16_t correction = next_bit(bs);
                    debug_print("%d", (int) correction);
                    correction = correction << sh->al;

                    if (du[i] > 0) {
                        du[i] += 1 << sh->al;
                    } else {
                        du[i] +=  (-1) * (1 << sh->al);
                    }
                }
                i++;
            }
            debug_print("\n");

            debug_print("\n du write index %d %x %x %d\n", i, curr_code, mag, amp);
            if (du[i] != 0 || i > 63) {
                debug_print("ERROR OCCURED DU[I] SHOULD BE ZERO\n");
                return -1;
            }
            du[i] = amp;
        } else { //size is zero check for ZRL and EOB
            //Determine the number of blocks to skip
            if (run < 0xF) {
                uint16_t eob = 1;
                if (run > 0) {
                    debug_print("eob bits ");
                }
                for (int j = 0; j < run; j++) {
                    eob = (eob << 1) + next_bit(bs);
                }
                debug_print("\n");

                if (i <= sh->se && du[i] != 0) {
                    debug_print("refine_bits ");
                }
                while (i <= sh->se) {
                    if (du[i] != 0) {
                        uint16_t correction = next_bit(bs);
                        debug_print("%d", (int) correction);
                        correction = correction << sh->al;

                        if (du[i] > 0) {
                            du[i] += 1 << sh->al;
                        } else {
                            du[i] += (-1) * (1 << sh->al);
                        }
                    }
                    i++;
                }
                debug_print("\n");

                debug_print("additional end of blocks %d\n", eob - 1);
                return eob - 1;
            } else if (run == 0xF) { //ZRL
                int j = 0;
                debug_print("correction: ");
                while (j < 15) { //Goes through 16 zero run length numbers
                    if (du[i] == 0) {
                        j++;
                    } else {
                        uint16_t correction = next_bit(bs);
                        debug_print("%d", (int) correction);
                        correction = correction << sh->al;

                        if (du[i] > 0) {
                            du[i] += 1 << sh->al;
                        } else {
                            du[i] += (-1) * (1 << sh->al);
                        }
                    }
                    i++;
                }
                debug_print("\n");
            } 
        }
    }

    return 0;
}

int decode_correction(
    Bitstream *bs,
    int16_t *du,
    ScanHeader *sh
) {
    debug_print("eob correction: ");
    for (int i = sh->ss; i <= sh->se; i++) {
        if (du[i] != 0) {
            uint16_t correction = next_bit(bs);
            debug_print("%d", (int) correction);
            correction = correction << sh->al;

            if (du[i] > 0) {
                du[i] += 1 << sh->al;
            } else {
                du[i] += (-1) * (1 << sh->al);
            }
        }
    }
    debug_print("\n");
    return 0;
}

void restart_marker(int16_t *pred, uint8_t len) {
    for (int i = 0; i < len; i++) {
        *(pred + i) = 0;
    }
}

void print_image(Image *img) {
    debug_print("print_image ");
    for (int i = 0; i < img->n_du; i++) {
        debug_print("%d ", **(img->buf + i));
        debug_print("%d ", *(*(img->buf + i) + 1));
        debug_print("%d ", *(*(img->buf + i) + 2));
    }
    debug_print("\n");
}
