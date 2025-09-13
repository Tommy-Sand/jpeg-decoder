#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "frame_header.h"
#include "huff_table.h"
#include "quant_table.h"
#include "restart_interval.h"
#include "scan_header.h"

typedef struct {
    uint8_t **buf;
    int *width;
    uint16_t mcu;  //To track which mcu the Image is at is on
    uint8_t n_du;  //The number of components;
} Image;

Image *allocate_img(FrameHeader *fh);
void free_img(Image *img);
int decode_jpeg_buffer(uint8_t *buf, size_t len, FrameHeader *fh, Image **img);
