#include "frame_header.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "debug.h"

int32_t decode_frame_header(
    Encoding encoding_process,
    Bitstream *bs,
    FrameHeader *fh
) {
    uint16_t len = (next_byte(bs) << 8) + next_byte(bs);
    const uint16_t min_len = 8;
    if (len < min_len) {
        return -1;
    }

    fh->process = encoding_process;
    fh->precision = next_byte(bs);
    fh->Y = next_byte(bs) << 8;
    fh->Y += next_byte(bs);
    fh->X = next_byte(bs) << 8;
    fh->X += next_byte(bs);
    fh->ncs = next_byte(bs);
    if ((fh->ncs == 0) || (fh->ncs * 3 != len - 8)) {
        //No components no image
        return -1;
    }
    fh->cs = (Component *) malloc(fh->ncs * sizeof(Component));
    uint8_t max_hsf = 0;
    uint8_t max_vsf = 0;
    for (uint8_t i = 0; i < fh->ncs; i++) {
        Component *c = fh->cs + i;

        c->id = next_byte(bs);

        c->hsf = get_byte(bs) >> 4;
        if (c->hsf > max_hsf) {
            max_hsf = c->hsf;
        }
        c->vsf = next_byte(bs) & 0xF;
        if (c->vsf > max_vsf) {
            max_vsf = c->vsf;
        }
        c->qtid = next_byte(bs);
    }
    for (uint8_t i = 0; i < fh->ncs; i++) {
        Component *c = fh->cs + i;

        uint16_t x = ((uint16_t) ceil(fh->X * ((float) c->hsf / max_hsf)));
        c->x = x;
        uint16_t y = (uint16_t) ceil(fh->Y * ((float) c->vsf / max_vsf));
        c->y = y;
    }
    return 0;
}

int32_t decode_number_of_lines(Bitstream *bs, FrameHeader *fh) {
    if (fh == NULL) {
        return -1;
    }

    uint16_t len = next_byte(bs) << 8;
    len += next_byte(bs);
    if (len != 4) {
        return -1;
    }

    fh->Y = next_byte(bs) << 8;
    fh->Y += next_byte(bs);

    return 0;
}

int32_t free_frame_header(FrameHeader *fh) {
    if (fh->cs != NULL) {
        free(fh->cs);
    }
    return 0;
}

void print_frame_header(FrameHeader *fh) {
    if (!fh || !DEBUG) {
        return;
    }

    fprintf(
        stderr,
        "Frame Header\n"
        "Precision: %d,\n"
        "X: %d,\n"
        "Y: %d,\n"
        "ncs: %d,\n"
        "process: %s,\n",
        (int) fh->precision,
        fh->X,
        fh->Y,
        fh->ncs,
        encoding_str(fh->process)
    );

    print_component(fh->cs, fh->ncs);
}

void print_component(Component *comp, int len) {
    if (!comp || !DEBUG) {
        return;
    }

    for (int i = 0; i < len; i++) {
        Component *c = comp + i;
        fprintf(
            stderr,
            "    Component: %d,\n"
            "        Horizontal Sampling Factor: %d,\n"
            "        Vertical Sampling Factor: %d,\n"
            "        x: %d,\n"
            "        y: %d,\n"
            "        Quantization Table Id: %d,\n",
            c->id,
            c->hsf,
            c->vsf,
            c->x,
            c->y,
            c->qtid
        );
    }
}

char *encoding_str(Encoding process) {
    switch (process) {
        case BDCT:
            return "Baseline DCT";
        case ESDCTHC:
            return "Extended Sequential DCT Huffman Coding";
        case PDCTHC:
            return "Progressive DCT Huffman Coding";
        case LSHC:
            return "Lossless (Sequential) Huffman Coding";
        case DSDCTHC:
            return "Differential Sequential DCT Huffman Coding";
        case DPDCTHC:
            return "Differential Progressive DCT Huffman Coding";
        case DLSHC:
            return "Differential Lossless (Sequential) Huffman Coding";
        case ESDCTAC:
            return "Extended sequential DCT Arithmetic Coding";
        case PDCTAC:
            return "Progressive DCT Arithmetic Coding";
        case LSAC:
            return "Lossless (sequential) Arithmetic Coding";
        case DSDCTAC:
            return "Differential Sequential DCT Arithmetic Coding";
        case DPDCTAC:
            return "Differential Progressive DCT Arithmetic Coding";
        case DLSAC:
            return "Differential Lossless (Sequential) Arithmetic Coding";
    }
    return "Unknown encoding scheme";
}
