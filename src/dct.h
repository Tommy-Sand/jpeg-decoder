#pragma once
#include <complex.h>
#include <stdint.h>

void fast_2ddct(int16_t du[64], complex double ret_du[64]);
void fast_dct(complex double in[8], complex double out[8]);
void fft(uint8_t len, complex double data[len], uint8_t stride, complex double ret_du[len]);

void fast_2didct(int16_t du[64], uint8_t precision);
void fast_idct(complex double in[8], complex double out[8]);
