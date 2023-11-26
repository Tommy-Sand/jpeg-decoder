#pragma once
#include <stdint.h>

typedef enum {
	BDCT = 0,
	ESDCTHC = 1,
	PDCTHC = 2,
	LSHC = 3,
	ESDCTAC = 9,
	PDCTAC = 10,
	LSAC = 11,
} Encoding ;

typedef struct {
	uint8_t id;
	uint8_t hsf; //Horizontal Sampling Factor
	uint8_t vsf; //Vertical Sampling Factor
	uint16_t x; //See A.1.1 in spec for definition
	uint16_t y; //See A.1.1 in spec for definition
	uint8_t qtid; //Quantization Table 
} Component;

typedef struct {
	Encoding process;
	uint8_t precision;
	uint16_t X;
	uint16_t Y;
	uint8_t ncs; //number of components
	Component *cs; //components
} FrameHeader;

FrameHeader *new_frame_header();

int32_t decode_frame_header(Encoding encoding_process, uint8_t **encoded_data, FrameHeader *fh);

int32_t decode_number_of_lines(uint8_t **encoded_data, FrameHeader *fh);

int32_t free_frame_header (FrameHeader *fh); 

void print_frame_header(FrameHeader *fh);

void print_component(Component *comp, int len);
