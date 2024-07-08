#include "frame_header.h"
#include "decode.h"

int y_rgb(FrameHeader *fh, Image *img, uint8_t *pixels, uint16_t width, uint16_t height, uint16_t pitch);
int ycbcr_rgb(FrameHeader *fh, Image *img, uint8_t *pixels, uint16_t width, uint16_t height, uint16_t pitch);
int yccb_rgb(FrameHeader *fh, Image *img, uint8_t *pixels, uint16_t width, uint16_t height, uint16_t pitch);
int get_max_sf(FrameHeader *fh, uint8_t *max_hsf, uint8_t *max_vsf);
