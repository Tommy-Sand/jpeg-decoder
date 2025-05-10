#include "encode.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "dct.h"


int encode_jpeg_buffer(
	uint8_t *buf, 
	size_t width, 
	size_t height, 
	size_t bitdepth, 
	uint8_t colorspace
) {
	//Accessing the buf means taking the 
	uint64_t size = width * height * bitdepth;
	int16_t y[64];
	int16_t cb[64];
	int16_t cr[64];
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j< 8; j++) {
			y[i * 8 + j] = (0.299 * buf[(i * width * (bitdepth/8)) + (j * (bitdepth/8))]) + 
				(0.587 * buf[(i * width * (bitdepth/8)) + (j * (bitdepth/8)) + 1]) + 
				(0.114 * buf[(i * width * (bitdepth/8)) + (j * (bitdepth/8)) + 2]);
			cb[i * 8 + j] = 128 - 
				(0.168736 * buf[(i * width * (bitdepth/8)) + (j * (bitdepth/8))]) - 
				(0.331264 * buf[(i * width * (bitdepth/8)) + (j * (bitdepth/8)) + 1]) + 
				(0.5 * buf[(i * width * (bitdepth/8)) + (j * (bitdepth/8)) + 2]);
			cr[i] = 128 + 
				(0.5 * buf[(i * width * (bitdepth/8)) + (j * (bitdepth/8))]) - 
				(0.418688 * buf[(i * width * (bitdepth/8)) + (j * (bitdepth/8)) + 1]) - 
				(0.081312 * buf[(i * width * (bitdepth/8)) + (j * (bitdepth/8)) + 2]);
		}
	}

	complex double ret_du[64];
	fast_2ddct(y, ret_du);

	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			printf("%f + %fi", creal(ret_du[(i * 8) + j]), cimag(ret_du[(i * 8) + j]));
		}
	}
}
