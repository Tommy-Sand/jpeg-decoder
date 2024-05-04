#pragma once
#include <complex.h>
#include <stdbool.h>
#include <stdint.h>

#include "frame_header.h"
#include "huff_table.h"
#include "quant_table.h"
#include "restart_interval.h"
#include "scan_header.h"

typedef struct {
    uint8_t** buf;
    uint16_t mcu;  //To track which mcu the Image is at is on
    uint8_t n_du;  //The number of components;
} Image;

Image* allocate_img(FrameHeader* fh);
void free_img(Image* img);
//int decode_img(uint8_t **encoded_data, FrameHeader *fh, ScanHeader *sh, HuffTables *hts, QuantTables *qts);
//int write_data_unit(Image *img, int16_t *du, uint8_t horz, uint8_t vert, FrameHeader *fh, uint8_t comp);
int write_mcu(Image* img, int16_t (**mcu)[64], FrameHeader* fh);
int write_data_unit(
    Image* img,
    uint8_t comp,
    uint16_t x_to_mcu,
    int16_t* du,
    uint32_t x,
    uint32_t y
);
int decode_scan(
    uint8_t** encoded_data,
    Image* img,
    FrameHeader* fh,
    ScanHeader* sh,
    HuffTables* hts,
    QuantTables* qts,
    RestartInterval ri
);
int decode_data_unit(
    uint8_t** encoded_data,
    uint8_t* offset,
    int16_t* du,
    HuffTable ac,
    HuffTable dc,
    int16_t* pred
);
int32_t idct(int16_t* du);
void fast_2didct(int16_t du[64]);
void ifct(complex double in[8], complex double out[8]);
void ifft(complex double du[4], complex double ret_du[4]);
int next_byte_restart_marker(uint8_t** ptr, uint8_t* offset);
int next_byte(uint8_t** ptr, uint8_t* offset);
uint8_t check_marker(uint8_t** ptr);
void restart_marker(int16_t* pred, uint8_t len);
