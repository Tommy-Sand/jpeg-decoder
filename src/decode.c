#include "decode.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define CLAMP(in) ((in > 255.0) ? 255 : ((in < 0.0) ? 0.0 : in))

Image* allocate_img(FrameHeader* fh) {
    Image* img = calloc(1, sizeof(Image));
    if (!img) {
        return NULL;
    }
    img->buf = (uint8_t**) calloc(fh->ncs, sizeof(uint8_t*));
    if (!img->buf) {
        free(img);
        return NULL;
    }
    for (uint8_t i = 0; i < fh->ncs; i++) {
        Component* c = fh->cs + i;
        uint16_t x_to_mcu = c->x
            + ((c->x % (8 * c->hsf)) ? (8 * c->hsf - (c->x % (8 * c->hsf))) : 0
            );
        uint16_t y_to_mcu = c->y
            + ((c->y % (8 * c->vsf)) ? (8 * c->vsf - (c->y % (8 * c->vsf))) : 0
            );
        img->buf[i] = (uint8_t*) calloc(x_to_mcu * y_to_mcu, sizeof(uint8_t));
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

void free_img(Image* img) {
    for (uint8_t i = 0; i < img->n_du; i++) {
        free(img->buf[i]);
    }
    free(img->buf);
    free(img);
}

int decode_jpeg_buffer(
    uint8_t* buf,
    size_t len,
    FrameHeader** fh_,
    Image** img_
) {
    FrameHeader* fh = new_frame_header();
    if (fh == NULL) {
        printf("Cannot malloc fh");
        return -1;
    }

    fh->process = BDCT;
    ScanHeader sh;
    Image* img = NULL;
    QuantTables* qts = NULL;
    HuffTables* hts = NULL;
    RestartInterval ri = 0;

    for (uint8_t* ptr = buf; ptr < buf + len;) {
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
                    printf("DEBUG: SOF Baseline DCT\n");
                    if (fh == NULL) {
                        //malloc fail check in decode_frame_header
                        fh = new_frame_header();
                        if (fh == NULL) {
                            printf("Cannot malloc fh");
                            return -1;
                        }
                    }
                    if (decode_frame_header(BDCT, &ptr, fh) == -1) {
                        printf("DEBUG: frame header read failed\n");
                        return -1;
                    }
                    print_frame_header(fh);
                    if (img == NULL) {
                        img = allocate_img(fh);
                        if (!img) {
                            printf("Cannot allocate image");
                            return -1;
                        }
                    }
                    break;
                case 0xC1:  //SOF Extended Sequential DCT
                    printf("DEBUG: SOF Extended Sequential DCT\n");
                    break;
                case 0xC2:  //SOF Progressive DCT
                    printf("DEBUG: SOF Progressive DCT\n");
                    break;
                case 0xC3:  //SOF Lossless Sequential
                    printf("DEBUG: SOF Lossless DCT\n");
                    break;
                case 0xC5:  //SOF Differential sequential DCT
                    printf("DEBUG: SOF Differential Sequential DCT\n");
                    break;
                case 0xC6:  //SOF Differential progressive DCT
                    printf("DEBUG: SOF Differential Progressive DCT\n");
                    break;
                case 0xC7:  //SOF Differential lossless (sequential)
                    printf("DEBUG: SOF Differential lossless DCT\n");
                    break;

                case 0xC8:  //SOF Reserved for JPEG extensions
                    printf("DEBUG: SOF Reserved for JPEG extensions\n");
                    break;
                case 0xC9:  //SOF Extended sequential DCT
                    printf("DEBUG: SOF Reserved for JPEG extensions\n");
                    break;
                case 0xCA:  //SOF Progressive DCT
                    printf("DEBUG: SOF Progressive DCT\n");
                    break;
                case 0xCB:  //SOF Lossless (sequential)
                    printf("SOF Lossless (sequential)\n");
                    break;

                case 0xCD:  //SOF Differential sequential DCT
                    printf("DEBUG: SOF Differential sequential DCT\n");
                    break;
                case 0xCE:  //SOF Differential progressive DCT
                    printf("DEBUG: SOF Differential progressive DCT\n");
                    break;
                case 0xCF:  //SOF Differential lossless (sequential)
                    printf("DEBUG: SOF Differential lossless\n");
                    break;

                case 0xC4:  //Define Huffman table(s)
                    printf("DEBUG: Define Huffman Table(s)\n");
                    if (fh == NULL) {
                        return -1;
                    }
                    if (hts == NULL) {
                        hts = new_huff_tables(fh->process);
                        if (hts == NULL) {
                            printf("Cannot malloc hts");
                            return -1;
                        }
                    }
                    if (decode_huff_tables(&ptr, hts) != 0) {
                        printf("DEBUG: Huff Table read failed\n");
                        return -1;
                    }
                    break;

                case 0xCC:  //Define arithmetic coding conditioning(s)
                    printf("DEBUG: Define Arithmetic coding conditioning(s)\n");
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
                    printf("DEBUG: Restart marker\n");
                    break;
                case 0xD8:  //SOI
                    printf("DEBUG: SOI\n");
                    break;
                case 0xD9:  //EOI
                    printf("DEBUG: EOI\n");
                    //return 0;
                    EOI = true;
                    break;
                case 0xDA:  //SOS
                    printf("DEBUG: SOS\n");

                    if (decode_scan_header(&ptr, &sh) != 0) {
                        printf("DEBUG: Scan Header read failed\n");
                        return -1;
                    }

                    print_scan_header(&sh);
                    if (decode_scan(&ptr, img, fh, &sh, hts, qts, ri) != 0) {
                        printf("DEBUG: Decode Scan failed\n");
                        return -1;
                    }
                    break;
                case 0xDB:  //Define Quant Table
                    printf("DEBUG: Definition of quant table\n");
                    if (qts == NULL) {
                        qts = new_quant_tables();
                        if (qts == NULL) {
                            printf("Cannot allocate memory for qts");
                            return -1;
                        }
                    }
                    if (decode_quant_table(&ptr, qts) == -1) {
                        printf("DEBUG: Quant Table read failed\n");
                    }
                    break;
                case 0xDC:  //Define number of lines
                    printf("DEBUG: Defined Number of lines\n");
                    if (fh == NULL) {
                        printf("DEBUG: fh is null when reading DNL");
                    }
                    if (decode_number_of_lines(&ptr, fh) != 0) {
                        printf("DEBUG: Number of Lines read fialed\n");
                    }
                    break;
                case 0xDD:  //Define restart interval
                    printf("DEBUG: Define restart interval\n");
                    if (decode_restart_interval(&ptr, &ri) != 0) {
                        printf("DEBUG: Restart Interval read failed\n");
                        return -1;
                    }

                    break;
                case 0xDE:  //Define Hierarchical Progression
                    printf("DEBUG: Hierarchical progression\n");
                    break;
                case 0xDF:  //Expand reference components
                    printf("DEBUG: Expand reference components\n");
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
                    printf("DEBUG: APPLICATION Segment\n");
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
                    printf("DEBUG: Reserved JPEG extensions\n");
                    break;

                case 0xFE:  //Comment
                    printf("DEBUG: Comment\n");
                    break;
            }
        }

        if (EOI) {
            break;
        }
    }

    *fh_ = fh;
    *img_ = img;
    if (qts != NULL) {
        free_quant_tables(qts);
    }
    if (hts != NULL) {
        free_huff_tables(hts);
    }
    return 0;
}

int read_app_segment(uint8_t** encoded_data) {
    uint8_t* ptr = *encoded_data;

    uint16_t len = (*(ptr++)) << 8;
    len += *(ptr++);

    *encoded_data += len;
    return 0;
}

int write_mcu(Image* img, int16_t (**mcu)[64], FrameHeader* fh) {
    for (uint8_t i = 0; i < fh->ncs; i++) {
        Component* c = fh->cs + i;
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

                int16_t* du = *((*(mcu + i)) + ((j * c->hsf) + k));

                write_data_unit(img, i, x_to_mcu, du, x, y);
            }
        }
    }
    img->mcu++;
    return 0;
}

int write_data_unit(
    Image* img,
    uint8_t comp,
    uint16_t x_to_mcu,
    int16_t* du,
    uint32_t x,
    uint32_t y
) {
    for (uint16_t j = y; j < y + 8; j++) {
        for (uint16_t k = x; k < x + 8; k++) {
            *(*(img->buf + comp) + (j * x_to_mcu) + k) =
                (uint8_t) du[((j - y) * 8) + (k - x)];
        }
    }
    return 0;
}

int decode_scan(
    uint8_t** encoded_data,
    Image* img,
    FrameHeader* fh,
    ScanHeader* sh,
    HuffTables* hts,
    QuantTables* qts,
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
    int16_t(**mcu)[] = (int16_t(**)[]) calloc(sh->nics, sizeof(int16_t**));
    if (!mcu) {
        return -1;
    }
    for (uint8_t i = 0; i < sh->nics; i++) {
        ImageComponent ic = sh->ics[i];

        Component* c = NULL;
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
            printf("End Reached");
            EOI = 1;
            break;
        }

        for (uint8_t i = 0; i < sh->nics; i++) {
            int16_t(*dus)[64] = *(mcu + i);
            ImageComponent ic = sh->ics[i];

            Component* c = NULL;
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
            QuantTable* qt = qts->tables + c->qtid;
            for (uint8_t j = 0; j < c->vsf; j++) {
                for (uint8_t k = 0; k < c->hsf; k++) {
                    int16_t* du = *(dus + ((j * c->hsf) + k));
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

                    fast_2didct(du);
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
    uint8_t** encoded_data,
    uint8_t* offset,
    int16_t* du,
    HuffTable dc_huff,
    HuffTable ac_huff,
    int16_t* pred
) {
    uint8_t* ptr = *encoded_data;

    if (*offset >= 8) {
        next_byte(&ptr, offset);
    }

    int16_t curr_code = (*ptr >> (7 - ((*offset)++))) & 0x1;
    if (*offset >= 8) {
        next_byte(&ptr, offset);
    }
    uint8_t mag = 0;
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
        dc += (-1 << mag) + 1;
    }

    dc += *pred;
    *pred = dc;
    du[0] = dc;

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
            amp += (-1 << size) + 1;
        }
        du[i] = amp;
    }

    *encoded_data = ptr;
    return 0;
}

void fast_2didct(int16_t du[64]) {
    complex double cdu[64] = {0.0};
    for (uint8_t i = 0; i < 64; i++) {
        cdu[i] = (complex double) du[i];
    }
    for (uint8_t i = 0; i < 8; i++) {
        cdu[i] *= 0.707106781;
        cdu[i * 8] *= 0.707106781;
    }

    complex double ret_dux[64] = {0};
    for (uint8_t i = 0; i < 8; i++) {
        ifct(cdu + (i * 8), ret_dux + (i * 8));
    }

    complex double in_duy[64] = {0};
    for (uint8_t j = 0; j < 8; j++) {
        in_duy[j * 8] = ret_dux[j];
        in_duy[(j * 8) + 1] = ret_dux[j + 8];
        in_duy[(j * 8) + 2] = ret_dux[j + 16];
        in_duy[(j * 8) + 3] = ret_dux[j + 24];
        in_duy[(j * 8) + 4] = ret_dux[j + 32];
        in_duy[(j * 8) + 5] = ret_dux[j + 40];
        in_duy[(j * 8) + 6] = ret_dux[j + 48];
        in_duy[(j * 8) + 7] = ret_dux[j + 56];
    }

    complex double ret_duy[64] = {0};
    for (uint8_t k = 0; k < 8; k++) {
        ifct(in_duy + (k * 8), ret_duy + (k * 8));
    }

    // transpose
    for (uint8_t j = 0; j < 8; j++) {
        du[j] = (int16_t) CLAMP((0.25 * creal(ret_duy[j * 8])) + 128.0);
        du[j + 8] =
            (int16_t) CLAMP((0.25 * creal(ret_duy[(j * 8) + 1])) + 128.0);
        du[j + 16] =
            (int16_t) CLAMP((0.25 * creal(ret_duy[(j * 8) + 2])) + 128.0);
        du[j + 24] =
            (int16_t) CLAMP((0.25 * creal(ret_duy[(j * 8) + 3])) + 128.0);
        du[j + 32] =
            (int16_t) CLAMP((0.25 * creal(ret_duy[(j * 8) + 4])) + 128.0);
        du[j + 40] =
            (int16_t) CLAMP((0.25 * creal(ret_duy[(j * 8) + 5])) + 128.0);
        du[j + 48] =
            (int16_t) CLAMP((0.25 * creal(ret_duy[(j * 8) + 6])) + 128.0);
        du[j + 56] =
            (int16_t) CLAMP((0.25 * creal(ret_duy[(j * 8) + 7])) + 128.0);
    }
}

// This original function is kept commented out to serve as a
// reference since the used version is unrolled and precomputed
/*
//Start with a vector of 8 inputs
void ifct(complex double du[8], complex double ret_du[8]) {
	const float PI =  3.14159265358979323846;

	complex double temp_du[8];
	temp_du[0] = du[0];
	for (uint8_t k = 1; k < 8; k++) {
		temp_du[k] = 0.5 * cexp(I * PI * (float) k / 16.0) * (du[k] - I * du[8 -
k]);
	}

	complex double packed[4];
	for(uint8_t i = 0; i < 4; i++) {
		packed[i] = (0.5 * (temp_du[i] + temp_du[4 + i])) +
			 (0.5 * I * cexp (I * PI * 2.0 * i / 8.0) * (temp_du[i] - temp_du[4
+ i]));
	}

	complex double ifft_ret_du[4];
	ifft(packed, ifft_ret_du);

	complex double unpacked_du[8];
	for (uint8_t i = 0; i < 4; i++) {
		unpacked_du[2 * i] = 2 * creal(ifft_ret_du[i]);
		unpacked_du[2 * i + 1] = 2 * cimag(ifft_ret_du[i]);
	}

	for (uint8_t i = 0; i < 4; i++) {
		ret_du[2*i] = unpacked_du[i];
		ret_du[16 - 1 - (2 * (4 + i))] = unpacked_du[4 + i];
	}
}
*/

void ifct(complex double in[8], complex double out[8]) {
    complex double pass1[8] = {
        in[0],
        0.5 * (0.9807853 + 0.1950903 * I) * (in[1] - I * in[8 - 1]),
        0.5 * (0.9238800 + 0.3826830 * I) * (in[2] - I * in[8 - 2]),
        0.5 * (0.8314700 + 0.5555700 * I) * (in[3] - I * in[8 - 3]),
        0.5 * (0.7071070 + 0.7071070 * I) * (in[4] - I * in[8 - 4]),
        0.5 * (0.5555700 + 0.8314700 * I) * (in[5] - I * in[8 - 5]),
        0.5 * (0.3826830 + 0.9238800 * I) * (in[6] - I * in[8 - 6]),
        0.5 * (0.1950900 + 0.9807850 * I) * (in[7] - I * in[8 - 7])};

    complex double pass2[4] = {
        (0.5 * (pass1[0] + pass1[4])) + (0.5 * I * (pass1[0] - pass1[4])),
        (0.5 * (pass1[1] + pass1[5]))
            + (0.5 * I * (0.707107 + 0.707107 * I) * (pass1[1] - pass1[5])),
        (0.5 * (pass1[2] + pass1[6])) + (0.5 * I * I * (pass1[2] - pass1[6])),
        (0.5 * (pass1[3] + pass1[7]))
            + (0.5 * I * (-0.707107 + 0.707107 * I) * (pass1[3] - pass1[7]))};

    complex double pass3[4];
    complex double temp[4];
    // even
    temp[0] = pass2[0] + pass2[2];
    temp[1] = pass2[0] - pass2[2];

    // odd
    temp[2] = pass2[1] + pass2[3];
    temp[3] = pass2[1] - pass2[3];

    pass3[0] = temp[0] + temp[2];
    pass3[2] = temp[0] - temp[2];

    pass3[1] = temp[1] + I * temp[3];
    pass3[3] = temp[1] - I * temp[3];

    complex double pass4[8] = {
        2 * creal(pass3[0]),
        2 * cimag(pass3[0]),
        2 * creal(pass3[1]),
        2 * cimag(pass3[1]),
        2 * creal(pass3[2]),
        2 * cimag(pass3[2]),
        2 * creal(pass3[3]),
        2 * cimag(pass3[3])};

    out[0] = pass4[0];
    out[1] = pass4[7];
    out[2] = pass4[1];
    out[3] = pass4[6];
    out[4] = pass4[2];
    out[5] = pass4[5];
    out[6] = pass4[3];
    out[7] = pass4[4];
}

// This original function is kept commented out to serve as a
// reference since the used version is unrolled and precomputed
/*
void ifft(uint8_t len, complex double du[len], uint8_t stride, complex double
ret_du[len]) { const float PI =  3.14159265358979323846;

	if (len == 1) {
		ret_du[0] = du[0];
		return;
	} else {
		complex double copy_du[len];
		memcpy(copy_du, du, sizeof(complex double) * len);
		complex double temp_ret_du[len];

		ifft(len/2, du, 2 * stride, ret_du);
		ifft(len/2, du + stride, 2 * stride, ret_du + len/2);

		complex double copy_du2[len];
		memcpy(copy_du2, ret_du, sizeof(complex double) * len);
		for (uint8_t i = 0; i < len/2; i++) {
			complex double temp_ret = ret_du[len/2 + i];
			complex double temp1 = cexp((I * PI * (float) (2*i) / (float)
(len))); complex double temp2 = temp1 * temp_ret; temp_ret_du[i] = ret_du[i] +
temp2; temp_ret_du[len/2 + i] = ret_du[i] - temp2;
		}
		for (uint8_t i = 0; i < len; i++) {
			ret_du[i] = temp_ret_du[i];
		}
	}
}
*/

void ifft(complex double du[4], complex double ret_du[4]) {
    complex double temp[4];
    // even
    temp[0] = du[0] + du[2];
    temp[1] = du[0] - du[2];

    // odd
    temp[2] = du[1] + du[3];
    temp[3] = du[1] - du[3];

    ret_du[0] = temp[0] + temp[2];
    ret_du[2] = temp[0] - temp[2];

    ret_du[1] = temp[1] + I * temp[3];
    ret_du[3] = temp[1] - I * temp[3];
}

/**
 * ret: 0 next byte
 *      1 restart marker
 *      2 byte stuffing
 */
// TODO Handle possibility of multiple stuffed bytes in a row
int next_byte(uint8_t** ptr, uint8_t* offset) {
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
int next_byte_restart_marker(uint8_t** ptr, uint8_t* offset) {
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
uint8_t check_marker(uint8_t** ptr) {
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

void restart_marker(int16_t* pred, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        *(pred + i) = 0;
    }
}
