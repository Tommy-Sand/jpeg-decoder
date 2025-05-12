#include "decode.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "dct.h"
#include "debug.h"

int write_mcu(Image *img, int16_t (**mcu)[64], FrameHeader *fh);
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
    uint8_t **encoded_data,
    Image *img,
    FrameHeader *fh,
    ScanHeader *sh,
    HuffTables *hts,
    QuantTables *qts,
    RestartInterval ri
);
int decode_data_unit(
    uint8_t **encoded_data,
    uint8_t *offset,
    int16_t *du,
    HuffTable ac,
    HuffTable dc,
    int16_t *pred
);
int next_byte_restart_marker(uint8_t **ptr, uint8_t *offset);
int next_byte(uint8_t **ptr, uint8_t *offset);
uint8_t check_marker(uint8_t **ptr);
void restart_marker(int16_t *pred, uint8_t len);
int read_app_segment(uint8_t **encoded_data);

Image *allocate_img(FrameHeader *fh) {
    Image *img = calloc(1, sizeof(Image));
    if (!img) {
        return NULL;
    }
    img->buf = (uint8_t **) calloc(fh->ncs, sizeof(uint8_t *));
    if (!img->buf) {
        free(img);
        return NULL;
    }
    for (uint8_t i = 0; i < fh->ncs; i++) {
        Component *c = fh->cs + i;
        uint16_t x_to_mcu = c->x
            + ((c->x % (8 * c->hsf)) ? (8 * c->hsf - (c->x % (8 * c->hsf))) : 0
            );
        uint16_t y_to_mcu = c->y
            + ((c->y % (8 * c->vsf)) ? (8 * c->vsf - (c->y % (8 * c->vsf))) : 0
            );
        img->buf[i] = (uint8_t *) calloc(x_to_mcu * y_to_mcu, sizeof(uint8_t));
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
    for (uint8_t i = 0; i < img->n_du; i++) {
        free(img->buf[i]);
    }
    free(img->buf);
    free(img);
}

int decode_jpeg_buffer(uint8_t *buf, size_t len, FrameHeader *fh, Image **img) {
    if (fh == NULL) {
        printf("Frame Header provided is NULL\n");
        return -1;
    }

    ScanHeader sh;
    Image *img_ = NULL;
    bool init_qts = false;
    QuantTables qts;
    bool init_hts = false;
    HuffTables hts;
    RestartInterval ri = 0;

    for (uint8_t *ptr = buf; ptr < buf + len;) {
        bool EOI = false;
        if (*(ptr++) == 0xFF) {
            switch (*(ptr++)) {
                case 0x01:  //Temp private use in arithmetic coding
                //Markers from
                case 0x02:
                //Through
                case 0xBF:
                    //Are Reserved
                    break;

                case 0xC0:
                    debug_print("DEBUG: SOF Baseline DCT\n");
                    if (decode_frame_header(BDCT, &ptr, fh) == -1) {
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
                case 0xC1:  //SOF Extended Sequential DCT
                    debug_print("DEBUG: SOF Extended Sequential DCT\n");
                    if (decode_frame_header(ESDCTHC, &ptr, fh) == -1) {
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
                case 0xC2:  //SOF Progressive DCT
                    debug_print("DEBUG: SOF Progressive DCT\n");
                    return -1;
                    break;
                case 0xC3:  //SOF Lossless Sequential
                    debug_print("DEBUG: SOF Lossless DCT\n");
                    return -1;
                    break;
                case 0xC5:  //SOF Differential sequential DCT
                    debug_print("DEBUG: SOF Differential Sequential DCT\n");
                    return -1;
                    break;
                case 0xC6:  //SOF Differential progressive DCT
                    debug_print("DEBUG: SOF Differential Progressive DCT\n");
                    return -1;
                    break;
                case 0xC7:  //SOF Differential lossless (sequential)
                    debug_print("DEBUG: SOF Differential lossless DCT\n");
                    return -1;
                    break;

                case 0xC8:  //SOF Reserved for JPEG extensions
                    debug_print("DEBUG: SOF Reserved for JPEG extensions\n");
                    return -1;
                    break;
                case 0xC9:  //SOF Extended sequential DCT
                    debug_print("DEBUG: SOF Extended sequential DCT\n");
                    return -1;
                    break;
                case 0xCA:  //SOF Progressive DCT
                    debug_print("DEBUG: SOF Progressive DCT\n");
                    return -1;
                    break;
                case 0xCB:  //SOF Lossless (sequential)
                    debug_print("SOF Lossless (sequential)\n");
                    return -1;
                    break;

                case 0xCD:  //SOF Differential sequential DCT
                    debug_print("DEBUG: SOF Differential sequential DCT\n");
                    return -1;
                    break;
                case 0xCE:  //SOF Differential progressive DCT
                    debug_print("DEBUG: SOF Differential progressive DCT\n");
                    return -1;
                    break;
                case 0xCF:  //SOF Differential lossless (sequential)
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
                    if (decode_huff_tables(&ptr, &hts) != 0) {
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

                    if (decode_scan_header(&ptr, &sh) != 0) {
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
                    if (decode_scan(&ptr, img_, fh, &sh, &hts, &qts, ri) != 0) {
                        debug_print("DEBUG: Decode Scan failed\n");
                        return -1;
                    }
                    break;
                case 0xDB:  //Define Quant Table
                    debug_print("DEBUG: Definition of quant table\n");
                    if (decode_quant_table(&ptr, &qts) == -1) {
                        debug_print("DEBUG: Quant Table read failed\n");
                    }
                    init_qts = true;
                    break;
                case 0xDC:  //Define number of lines
                    debug_print("DEBUG: Defined Number of lines\n");
                    if (decode_number_of_lines(&ptr, fh) != 0) {
                        debug_print("DEBUG: Number of Lines read fialed\n");
                    }
                    break;
                case 0xDD:  //Define restart interval
                    debug_print("DEBUG: Define restart interval\n");
                    if (decode_restart_interval(&ptr, &ri) != 0) {
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
                    read_app_segment(&ptr);
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
    return 0;
}

int read_app_segment(uint8_t **encoded_data) {
    uint8_t *ptr = *encoded_data;

    uint16_t len = (*(ptr++)) << 8;
    len += *(ptr++);

    *encoded_data += len;
    return 0;
}

int write_mcu(Image *img, int16_t (**mcu)[64], FrameHeader *fh) {
    for (uint8_t i = 0; i < fh->ncs; i++) {
        Component *c = fh->cs + i;
        for (uint8_t j = 0; j < c->vsf; j++) {
            for (uint8_t k = 0; k < c->hsf; k++) {
                uint16_t x_to_mcu = c->x
                    + ((c->x % (8 * c->hsf))
                           ? (8 * c->hsf - (c->x % (8 * c->hsf)))
                           : 0);
                uint16_t mcu_progress = img->mcu;
                uint16_t x = ((mcu_progress * c->hsf * 8) + (k * 8)) % x_to_mcu;
                uint16_t y =
                    ((((mcu_progress * c->hsf * 8) + (k * 8)) / x_to_mcu)
                     * c->vsf * 8)
                    + (j * 8);

                int16_t *du = *((*(mcu + i)) + ((j * c->hsf) + k));

                write_data_unit(
                    img,
                    i,
                    x_to_mcu,
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
#pragma GCC unroll 8
    for (uint16_t j = y; j < y + 8; j++) {
#pragma GCC unroll 8
        for (uint16_t k = x; k < x + 8; k++) {
            if (precision) {
                /* TODO A precision of 12 bits is scaled down to 8 bits.
				 * This should be changed as we should support 12 bits of
				 * precision in the viewer */
                *(*(img->buf + comp) + (j * x_to_mcu) + k) =
                    (uint8_t) ((du[((j - y) * 8) + (k - x)] / 4096.0) * 255.0);
            } else {
                *(*(img->buf + comp) + (j * x_to_mcu) + k) =
                    (uint8_t) du[((j - y) * 8) + (k - x)];
            }
        }
    }
    return 0;
}

int decode_scan(
    uint8_t **encoded_data,
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
    uint8_t offset = 0;
    int16_t pred[sh->nics];
    for (uint8_t i = 0; i < sh->nics; i++) {
        pred[i] = 0;
    }

    uint8_t EOI = 0;
    int16_t(**mcu)[] = (int16_t(**)[]) calloc(sh->nics, sizeof(int16_t **));
    if (!mcu) {
        return -1;
    }
    for (uint8_t i = 0; i < sh->nics; i++) {
        ImageComponent ic = sh->ics[i];

        Component *c = NULL;
        for (uint8_t j = 0; j < fh->ncs; j++) {
            c = (fh->cs) + j;
            if (c->id == ic.sc) {
                break;
            }
        }
        if (c == NULL) {
            return -1;
        }
        int16_t(*dus)[64] =
            (int16_t(*)[64]) calloc(64 * (c->hsf * c->vsf), sizeof(int16_t));
        *(mcu + i) = dus;
    }

    uint32_t mcus_read = 0;
    while (!EOI) {
        uint8_t ret = check_marker(encoded_data);
        if ((ri != 0) && ((mcus_read % ri) == 0) && ret == 1) {
            if (ret == 1) {
                restart_marker(pred, sh->nics);
                next_byte_restart_marker(encoded_data, &offset);
            }
        } else if (ret == 2) {
            debug_print("End Reached");
            EOI = 1;
            break;
        }

        for (uint8_t i = 0; i < sh->nics; i++) {
            int16_t(*dus)[64] = *(mcu + i);
            ImageComponent ic = sh->ics[i];

            Component *c = NULL;
            for (uint8_t j = 0; j < fh->ncs; j++) {
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
            for (uint8_t j = 0; j < c->vsf; j++) {
                for (uint8_t k = 0; k < c->hsf; k++) {
                    int16_t *du = *(dus + ((j * c->hsf) + k));
                    memset(du, 0, 64 * sizeof(int16_t));
                    if (!du) {
                        return -1;
                    }
                    if (decode_data_unit(
                            encoded_data,
                            &offset,
                            du,
                            dc,
                            ac,
                            pred + i
                        )
                        != 0) {
                        free(mcu);
                        return -1;
                    }

                    if (dequant_data_unit(qt, du) != 0) {
                        free(mcu);
                        return -1;
                    }

                    fast_2didct(du, fh->precision == 12);
                }
            }
        }
        write_mcu(img, mcu, fh);
        mcus_read++;
    }

    for (uint8_t i = 0; i < sh->nics; i++) {
        free(*(mcu + i));
    }
    free(mcu);
    return 0;
}

int decode_data_unit(
    uint8_t **encoded_data,
    uint8_t *offset,
    int16_t *du,
    HuffTable dc_huff,
    HuffTable ac_huff,
    int16_t *pred
) {
    uint8_t *ptr = *encoded_data;

    if (*offset >= 8) {
        next_byte(&ptr, offset);
    }

    int16_t curr_code = (*ptr >> (7 - ((*offset)++))) & 0x1;
    if (*offset >= 8) {
        next_byte(&ptr, offset);
    }
    uint8_t mag = 0;
#pragma GCC unroll 16
    for (uint8_t i = 0; i < 16; i++) {
        if (dc_huff.symbols[i] == NULL) {
            curr_code =
                (curr_code << 1) + ((*ptr >> (7 - ((*offset)++))) & 0x1);
            if ((*offset) >= 8) {
                next_byte(&ptr, offset);
            }
            continue;
        }

        if (dc_huff.max_codes[i] >= curr_code) {
            mag = (uint8_t)
                * (dc_huff.symbols[i] + (curr_code - dc_huff.min_codes[i]));
            break;
        }
        curr_code = (curr_code << 1) + ((*ptr >> (7 - ((*offset)++))) & 0x1);
        if ((*offset) >= 8) {
            next_byte(&ptr, offset);
            *offset = 0;
        }
    }
    int16_t dc = 0;
    for (uint8_t j = 0; j < mag; j++) {
        dc = (dc << 1) + ((*ptr >> (7 - ((*offset)++))) & 0x1);
        if ((*offset) >= 8) {
            next_byte(&ptr, offset);
            (*offset) = 0;
        }
    }

    if (dc < (1 << (mag - 1))) {
        dc += -1 - ((1 << mag) - 2);
    }

    dc += *pred;
    *pred = dc;
    du[0] = dc;

#pragma GCC unroll 64
    for (uint8_t i = 1; i < 64; i++) {
        curr_code = (*ptr >> (7 - ((*offset)++))) & 0x1;
        if (*offset >= 8) {
            next_byte(&ptr, offset);
            *offset = 0;
        }
        for (uint8_t j = 0; j < 16; j++) {
            if (ac_huff.symbols[j] == NULL) {
                curr_code =
                    (curr_code << 1) + ((*ptr >> (7 - ((*offset)++))) & 0x1);
                if ((*offset) >= 8) {
                    next_byte(&ptr, offset);
                    *offset = 0;
                }
                continue;
            }

            if (ac_huff.max_codes[j] >= curr_code) {
                mag = (uint8_t)
                    * (ac_huff.symbols[j]
                       + (uint8_t) (curr_code - ac_huff.min_codes[j]));
                break;
            }
            curr_code =
                (curr_code << 1) + ((*ptr >> (7 - ((*offset)++))) & 0x1);
            if (*offset >= 8) {
                next_byte(&ptr, offset);
                *offset = 0;
            }
        }

        uint8_t size = mag % 16;
        uint8_t run = (mag >> 4) & 0xF;

        if (mag == 0xF0) {
            i += (0x10 - 1);
            continue;
        } else {
            i += run;
        }
        if (mag == 0x00) {
            break;
        }

        int16_t amp = 0;
        for (uint8_t j = 0; j < size; j++) {
            amp = (amp << 1) + ((*ptr >> (7 - ((*offset)++))) & 0x1);
            if (*offset >= 8) {
                next_byte(&ptr, offset);
                *offset = 0;
            }
        }

        if (amp < (1 << (size - 1))) {
            amp += -1 - ((1 << size) - 2);
        }
        du[i] = amp;
    }

    *encoded_data = ptr;
    return 0;
}

/**
 * ret: 0 next byte
 *      1 restart marker
 *      2 byte stuffing
 */
// TODO Handle possibility of multiple stuffed bytes in a row
int next_byte(uint8_t **ptr, uint8_t *offset) {
    uint8_t byte = **ptr;
    if (byte == 0xFF) {
        uint8_t n_byte = *(*ptr + 1);
        if (n_byte == 0x00) {
            // Byte is stuffed and we can skip the 00
            *ptr = *ptr + 2;
            *offset = 0;
            return 2;
        }
    } else {
        uint8_t n_byte = *(*ptr + 1);
        if (n_byte == 0xFF) {
            uint8_t nn_byte = *(*ptr + 2);
            /*
			if (nn_byte >= 0xD0 && nn_byte <= 0xD7) {
				*ptr += 3;
				*offset = 0;
				return 1;
			}
			else if (nn_byte == 0x00) {
			*/
            if (nn_byte == 0x00) {
                *ptr = *ptr + 1;
                *offset = 0;
                return 0;
            }
            // TODO handle DNL
        }
    }
    ++(*ptr);
    *offset = 0;
    return 0;
}

/**
 * ret: 0 next byte
 *      1 restart marker
 *      2 byte stuffing
 */
// TODO Handle possibility of multiple stuffed bytes in a row
inline int next_byte_restart_marker(uint8_t **ptr, uint8_t *offset) {
    uint8_t byte = **ptr;
    if (byte == 0xFF) {
        uint8_t n_byte = *(*ptr + 1);
        if (n_byte == 0x00) {
            // Byte is stuffed and we can skip the 00
            *ptr = *ptr + 2;
            *offset = 0;
            return 2;
        }
        if (n_byte >= 0xD0 && n_byte <= 0xD7) {
            *ptr += 2;
            *offset = 0;
            return 1;
        }
    } else {
        uint8_t n_byte = *(*ptr + 1);
        if (n_byte == 0xFF) {
            uint8_t nn_byte = *(*ptr + 2);
            if (nn_byte >= 0xD0 && nn_byte <= 0xD7) {
                *ptr += 3;
                *offset = 0;
                return 1;
            } else if (nn_byte == 0x00) {
                *ptr = *ptr + 1;
                *offset = 0;
                return 0;
            }
            // TODO handle DNL
        }
    }
    ++(*ptr);
    *offset = 0;
    return 0;
}

/*
	1 is restart marker
	2 end of image
*/
uint8_t check_marker(uint8_t **ptr) {
    uint8_t byte = **ptr;
    uint8_t n_byte = *(*ptr + 1);
    if (byte == 0xFF && n_byte == 0xD9) {
        return 2;
    }

    uint8_t nn_byte = *(*ptr + 2);
    if (byte == 0xFF) {
        if (n_byte >= 0xD0 && n_byte <= 0xD7) {
            return 1;
        }

        uint8_t nnn_byte = *(*ptr + 3);
        if (n_byte == 0x00 && nn_byte == 0xFF
            && (nnn_byte >= 0xD0 && nnn_byte <= 0xD7)) {
            return 1;
        } else if (n_byte == 0x00 && nn_byte == 0xFF && nnn_byte >= 0xD9) {
            return 2;
        }
    } else if (n_byte == 0xFF && (nn_byte >= 0xD0 && nn_byte <= 0xD7)) {
        return 1;
    } else if (n_byte == 0xFF && nn_byte == 0xD9) {
        return 2;
    }
    return 0;
}

void restart_marker(int16_t *pred, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        *(pred + i) = 0;
    }
}
