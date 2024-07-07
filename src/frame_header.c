#include "frame_header.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int32_t decode_frame_header(
    Encoding encoding_process,
    uint8_t **encoded_data,
    FrameHeader *fh
) {
    if (encoded_data == NULL || *encoded_data == NULL || fh == NULL) {
        return -1;
    }
    uint8_t *ptr = *encoded_data;

    uint16_t len = (*(ptr++)) << 8;
    len += *(ptr++);
    if (len < 8) {
        return -1;
    }

    fh->process = encoding_process;
    fh->precision = (*(ptr++));
    fh->Y = (*(ptr++)) << 8;
    fh->Y += *(ptr++);
    fh->X = (*(ptr++)) << 8;
    fh->X += *(ptr++);
    fh->ncs = *(ptr++);
    if ((fh->ncs == 0) || (fh->ncs * 3 != len - 8)) {
        //No components no image
        return -1;
    }
    fh->cs = (Component *) malloc(fh->ncs * sizeof(Component));
    uint8_t max_hsf = 0;
    uint8_t max_vsf = 0;
    for (uint8_t i = 0; i < fh->ncs; i++) {
        Component *c = fh->cs + i;

        c->id = *(ptr++);
        c->hsf = (*ptr) >> 4;
        if (c->hsf > max_hsf) {
            max_hsf = c->hsf;
        }
        c->vsf = (*(ptr++)) & 0xF;
        if (c->vsf > max_vsf) {
            max_vsf = c->vsf;
        }
        c->qtid = *(ptr++);
    }
    for (uint8_t i = 0; i < fh->ncs; i++) {
        Component *c = fh->cs + i;

        uint16_t x = ((uint16_t) ceil(fh->X * ((float) c->hsf / max_hsf)));
        //x += x % 8;
        c->x = x;
        uint16_t y = (uint16_t) ceil(fh->Y * ((float) c->vsf / max_vsf));
        //y += y % 8;
        c->y = y;
    }

    if (0) {
        printf("Percision: %d\n", fh->precision);
        printf("Y: %d\n", fh->Y);
        printf("X: %d\n", fh->X);
        printf("number of components: %d\n", fh->ncs);
        for (uint8_t i = 0; i < fh->ncs; i++) {
            printf("\tid: %d\n", fh->cs[i].id);
            printf("\thorizontal sampling factor: %d\n", fh->cs[i].hsf);
            printf("\tvertical sampling factor: %d\n", fh->cs[i].vsf);
            printf("\tx: %d\n", fh->cs[i].x);
            printf("\ty: %d\n", fh->cs[i].y);
            printf("Quantization table id: %d\n", fh->cs[i].qtid);
        }
    }

    *encoded_data = ptr;
    return 0;
}

int32_t decode_number_of_lines(uint8_t **encoded_data, FrameHeader *fh) {
    if (fh == NULL) {
        return -1;
    }
    uint8_t *ptr = *encoded_data;
    uint16_t len = (*(ptr++)) << 8;
    len += (*(ptr++));
    if (len != 4) {
        return -1;
    }

    fh->Y = (*(ptr++)) << 8;
    fh->Y += (*(ptr++));

    return 0;
}

int32_t free_frame_header(FrameHeader *fh) {
    if (fh->cs != NULL) {
        free(fh->cs);
    }
    return 0;
}

void print_frame_header(FrameHeader *fh) {
    if (!fh) {
        return;
    }

    printf(
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
    if (!comp) {
        return;
    }

    for (int i = 0; i < len; i++) {
        Component *c = comp + i;
        printf(
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
