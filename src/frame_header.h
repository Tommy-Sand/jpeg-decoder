#pragma once
#include <stdint.h>

typedef enum {
    //non differential huffman coding
    BDCT = 0,  //Baseline DCT
    ESDCTHC = 1,  //Extended Sequential DCT
    PDCTHC = 2,  //Progrssive DCT
    LSHC = 3,  //Lossless (sequential)
    //Differential huffman coding
    DSDCTHC = 5,  //Differential Sequential DCT
    DPDCTHC = 6,  //Differential Progressive DCT
    DLSHC = 7,  //Differential lossless (sequential)
    //Non-differential arithmetic coding
    ESDCTAC = 9,  //Extended sequential DCT
    PDCTAC = 10,  //Progressive DCT
    LSAC = 11,  //Lossless (sequential)
    //Differential arithmetic coding
    DSDCTAC = 13,  //Differential Sequential DCT
    DPDCTAC = 14,  //Differential Progressive DCT
    DLSAC = 15,  //Differential lossless (sequential)
} Encoding;

char *encoding_str(Encoding process);

typedef struct {
    uint8_t id;
    uint8_t hsf;  //Horizontal Sampling Factor
    uint8_t vsf;  //Vertical Sampling Factor
    uint16_t x;  //See A.1.1 in spec for definition
    uint16_t y;  //See A.1.1 in spec for definition
    uint8_t qtid;  //Quantization Table
} Component;

typedef struct {
    Encoding process;
    uint8_t precision; 
    uint16_t X;
    uint16_t Y;
    uint8_t ncs;  //number of components
    Component *cs;  //components
} FrameHeader;

int32_t decode_frame_header(
    Encoding encoding_process,
    uint8_t **encoded_data,
    FrameHeader *fh
);

int32_t decode_number_of_lines(uint8_t **encoded_data, FrameHeader *fh);

int32_t free_frame_header(FrameHeader *fh);

void print_frame_header(FrameHeader *fh);

void print_component(Component *comp, int len);
