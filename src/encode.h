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
    uint8_t *buf;
	size_t width; // The width of the image
	size_t height; // The height of the image
	size_t bitdepth; // Should be 3 * 8
	uint8_t color_space; //The color space of the buffer
	uint8_t pixel_size; //Unused for now saving for 12 bit percision
} EncodeStruct;

int encode_jpeg_buffer(
	uint8_t *buf, 
	size_t width, 
	size_t height, 
	size_t bitdepth, 
	uint8_t colorspace
);
