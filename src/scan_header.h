#pragma once
#include <stdint.h>

typedef struct {
	uint8_t sc;
	uint8_t dc;
	uint8_t ac;
} ImageComponent;

typedef struct {
	uint8_t nics; //number of image components in scan 
	ImageComponent *ics;
	uint8_t ss;
	uint8_t se;
	uint8_t ah;
	uint8_t al;
} ScanHeader;

ScanHeader *new_scan_header();

int32_t decode_scan_header(uint8_t **encoded_data, ScanHeader *sh);

int32_t free_scan_header(ScanHeader *sh);

void print_scan_header(ScanHeader *sh);

void print_image_component(ImageComponent *ics, int len);
