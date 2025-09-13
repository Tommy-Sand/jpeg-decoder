#include "dct.h"

#include <stdio.h>
#include <string.h>

#define CLAMP_8(in) ((in > 255.0) ? 255 : ((in < 0.0) ? 0.0 : in))
#define CLAMP_16(in) ((in > 65535.0) ? 65535.0 : ((in < 0.0) ? 0.0 : in))

void fast_2ddct(int16_t du[64], complex double ret_du[64]) {
    complex double cdu[64] = {0.0};
    for (uint8_t i = 0; i < 64; i++) {
        cdu[i] = (complex double) (du[i] - 128);
    }

    complex double ret_dux[64] = {0};
    for (uint8_t i = 0; i < 8; i++) {
        fast_dct(cdu + (i * 8), ret_dux + (i * 8));
    }

    complex double in_duy[64] = {0};
    for (uint8_t j = 0; j < 8; j++) {
        in_duy[j * 8] = ret_dux[j];
        in_duy[(j * 8) + 1] = ret_dux[j + 8];
        in_duy[(j * 8) + 2] = ret_dux[j + 16];
        in_duy[(j * 8) + 3] = ret_dux[j + 24];
        in_duy[(j * 8) + 4] = ret_dux[j + 32];
        in_duy[(j * 8) + 5] = ret_dux[j + 40];
        in_duy[(j * 8) + 6] = ret_dux[j + 48];
        in_duy[(j * 8) + 7] = ret_dux[j + 56];
    }

    complex double ret_duy[64] = {0};
    for (uint8_t k = 0; k < 8; k++) {
        fast_dct(in_duy + (k * 8), ret_duy + (k * 8));
    }

    for (uint8_t j = 0; j < 8; j++) {
        ret_du[j] = creal(ret_duy[j * 8]);
        ret_du[j + 8] = creal(ret_duy[(j * 8) + 1]);
        ret_du[j + 16] = creal(ret_duy[(j * 8) + 2]);
        ret_du[j + 24] = creal(ret_duy[(j * 8) + 3]);
        ret_du[j + 32] = creal(ret_duy[(j * 8) + 4]);
        ret_du[j + 40] = creal(ret_duy[(j * 8) + 5]);
        ret_du[j + 48] = creal(ret_duy[(j * 8) + 6]);
        ret_du[j + 56] = creal(ret_duy[(j * 8) + 7]);
    }

    for (uint8_t i = 0; i < 64; i++) {
        ret_du[i] = (complex double) (0.25 * du[i]);
    }
    for (uint8_t i = 0; i < 8; i++) {
        ret_du[i] *= 0.707106781;
        ret_du[i * 8] *= 0.707106781;
    }
}

void correct_fast_dct(complex double in[8], complex double out[8]) {
    const float PI = 3.14159265358979323846;

    complex double temp_du[8];
    for (uint8_t i = 0; i < 4; i++) {
        temp_du[i] = in[2 * i];
        temp_du[4 + i] = in[16 - 2 * (4 + i) - 1];
    }

    complex double fft_ret_du[8];
    fft(8, temp_du, 1, fft_ret_du);

    for (uint8_t i = 0; i < 8; i++) {
        fft_ret_du[i] *= 2 * cexp(-I * PI * i / 16.0);
    }

    for (uint8_t i = 0; i < 5; i++) {
        out[i] = creal(fft_ret_du[i]);
        if (i > 0) {
            out[8 - i] = -cimag(fft_ret_du[i]);
        }
    }
}

void fast_dct(complex double in[8], complex double out[8]) {
    const float PI = 3.14159265358979323846;

    complex double temp_du[8];
    for (uint8_t i = 0; i < 4; i++) {
        temp_du[i] = in[2 * i];
        temp_du[4 + i] = in[16 - 2 * (4 + i) - 1];
    }

    complex double packed[4];
    for (uint8_t i = 0; i < 4; i++) {
        packed[i] = temp_du[2 * i] + I * temp_du[2 * i + 1];
    }

    complex double fft_ret_du[4];
    fft(4, packed, 1, fft_ret_du);

    complex double unpacked_du[8];
    for (uint8_t i = 1; i < 3; i++) {
        complex double temp1 = 0.5 * (fft_ret_du[i] + conj(fft_ret_du[4 - i]));
        complex double temp2 = 0.5 * I * cexp(I * -2.0 * i * PI / 8)
            * (fft_ret_du[i] - conj(fft_ret_du[4 - i]));
        unpacked_du[1] = temp1 - temp2;
    }
    unpacked_du[0] = creal(fft_ret_du[0]) + cimag(fft_ret_du[0]);
    unpacked_du[2] = creal(fft_ret_du[2]) - I * cimag(fft_ret_du[2]);
    unpacked_du[4] = creal(fft_ret_du[0]) - cimag(fft_ret_du[0]);

    for (uint8_t i = 0; i < 5; i++) {
        unpacked_du[i] = 2 * cexp(-I * PI * i / 16) * unpacked_du[i];
    }

    for (uint8_t i = 0; i < 5; i++) {
        out[i] = creal(unpacked_du[i]);
        if (i > 0) {
            out[8 - i] = -cimag(unpacked_du[i]);
        }
    }
}

void fft(
    uint8_t len,
    complex double du[len],
    uint8_t stride,
    complex double ret_du[len]
) {
    const float PI = 3.14159265358979323846;

    if (len == 1) {
        ret_du[0] = du[0];
        return;
    } else {
        complex double copy_du[len];
        memcpy(copy_du, du, sizeof(complex double) * len);
        complex double temp_ret_du[len];

        fft(len / 2, du, 2 * stride, ret_du);
        fft(len / 2, du + stride, 2 * stride, ret_du + len / 2);

        complex double copy_du2[len];
        memcpy(copy_du2, ret_du, sizeof(complex double) * len);
        for (uint8_t i = 0; i < len / 2; i++) {
            complex double temp_ret = ret_du[len / 2 + i];
            complex double temp1 =
                cexp((I * PI * (float) (-2 * i) / (float) len));
            complex double temp2 = temp1 * temp_ret;
            temp_ret_du[i] = ret_du[i] + temp2;
            temp_ret_du[len / 2 + i] = ret_du[i] - temp2;
        }
        for (uint8_t i = 0; i < len; i++) {
            ret_du[i] = temp_ret_du[i];
        }
    }
}

//precision: 0 indicates an 8 bit precision level, add 128
//precision: 1 indicates a 12 bit precision level, add 2048
inline void fast_2didct(int16_t du[64], uint8_t precision) {
    double cdu[64] = {0.0};
    for (uint8_t i = 0; i < 64; i++) {
        cdu[i] = (double) du[i];
    }

    for (uint8_t i = 0; i < 8; i++) {
        cdu[i] *= 0.707106781;
        cdu[i * 8] *= 0.707106781;
    }

    for (uint8_t i = 0; i < 8; i++) {
        fast_idct(cdu + (i * 8));
    }

    // transpose
    for (int j = 0; j < 8; j++) {
        for (int k = j + 1; k < 8; k++) {
            double temp = cdu[(8 * j) + k];
            cdu[(8 * j) + k] = cdu[(8 * k) + j];
            cdu[(8 * k) + j] = temp;
        }
    }

    for (uint8_t k = 0; k < 8; k++) {
        fast_idct(cdu + (k * 8));
    }

    // transpose
    double level_shift = 128.0;
    if (!precision) {
        for (uint8_t j = 0; j < 8; j++) {
            for (uint8_t k = 0; k < 8; k++) {
                du[j + (8 * k)] =
                    (int16_t) CLAMP_8((0.25 * cdu[(j * 8) + k]) + level_shift);
            }
        }
    } else {
        double level_shift = 2048.0;
        for (uint8_t j = 0; j < 8; j++) {
            for (uint8_t k = 0; k < 8; k++) {
                du[j + (8 * k)] =
                    (int16_t) CLAMP_16((0.25 * cdu[(j * 8) + k]) + level_shift);
            }
        }
    }
}

// This original function is kept commented out to serve as a
// reference since the used version is unrolled and precomputed
/*
//Start with a vector of 8 inputs
void ifct(complex double du[8], complex double ret_du[8]) {
	const float PI =  3.14159265358979323846;

	complex double temp_du[8];
	temp_du[0] = du[0];
	for (uint8_t k = 1; k < 8; k++) {
		temp_du[k] = 0.5 * cexp(I * PI * (float) k / 16.0) * (du[k] - I * du[8 - k]);
	}

	complex double packed[4];
	for(uint8_t i = 0; i < 4; i++) {
		packed[i] = (0.5 * (temp_du[i] + temp_du[4 + i])) +
			 (0.5 * I * cexp (I * PI * 2.0 * i / 8.0) * (temp_du[i] - temp_du[4 + i]));
	}

	complex double ifft_ret_du[4];
	ifft(packed, ifft_ret_du);

	complex double unpacked_du[8];
	for (uint8_t i = 0; i < 4; i++) {
		unpacked_du[2 * i] = 2 * creal(ifft_ret_du[i]);
		unpacked_du[2 * i + 1] = 2 * cimag(ifft_ret_du[i]);
	}

	for (uint8_t i = 0; i < 4; i++) {
		ret_du[2*i] = unpacked_du[i];
		ret_du[16 - 1 - (2 * (4 + i))] = unpacked_du[4 + i];
	}
}
*/

//inline void fast_idct(complex double in[8], complex double out[8]) {
inline void fast_idct(double du[8]) {
    complex double pass1[8] = {
        du[0],
        ((0.4903927 * du[1]) + (0.0975452 * du[7]))
            - ((0.4903927 * du[7]) - (0.0975452 * du[1])) * I,
        ((0.46194 * du[2]) + (0.1913415 * du[6]))
            - ((0.46194 * du[6]) - (0.1913415 * du[2])) * I,
        ((0.415735 * du[3]) + (0.277785 * du[5]))
            - ((0.415735 * du[5]) - (0.277785 * du[3])) * I,
        ((0.3535535 * du[4]) + (0.3535535 * du[4]))
            - ((0.3535535 * du[4]) - (0.3535535 * du[4])) * I,
        ((0.277785 * du[5]) + (0.415735 * du[3]))
            - ((0.277785 * du[3]) - (0.415735 * du[5])) * I,
        ((0.1913415 * du[6]) + (0.46194 * du[2]))
            - ((0.1913415 * du[2]) - (0.46194 * du[6])) * I,
        ((0.0975452 * du[7]) + (0.4903927 * du[1]))
            - ((0.0975452 * du[1]) - (0.4903927 * du[7])) * I};

    complex double pass2[4] = {
        0.5 * ((pass1[0] + pass1[4]) + (pass1[0] - pass1[4]) * I),
        0.5
            * ((pass1[1] + pass1[5])
               + ((-0.707107 + 0.707107 * I) * (pass1[1] - pass1[5]))),
        0.5 * ((pass1[2] + pass1[6]) - (pass1[2] - pass1[6])),
        0.5
            * ((pass1[3] + pass1[7])
               + ((-0.707107 - 0.707107 * I) * (pass1[3] - pass1[7])))};

    complex double pass3[4];
    complex double temp[4];
    // even
    temp[0] = pass2[0] + pass2[2];
    temp[1] = pass2[0] - pass2[2];

    // odd
    temp[2] = pass2[1] + pass2[3];
    temp[3] = pass2[1] - pass2[3];

    pass3[0] = temp[0] + temp[2];
    pass3[2] = temp[0] - temp[2];

    pass3[1] = temp[1] + I * temp[3];
    pass3[3] = temp[1] - I * temp[3];

    du[0] = 2 * creal(pass3[0]);
    du[1] = 2 * cimag(pass3[3]);
    du[2] = 2 * cimag(pass3[0]);
    du[3] = 2 * creal(pass3[3]);
    du[4] = 2 * creal(pass3[1]);
    du[5] = 2 * cimag(pass3[2]);
    du[6] = 2 * cimag(pass3[1]);
    du[7] = 2 * creal(pass3[2]);
}

// This original function is kept commented out to serve as a
// reference since the used version is unrolled and precomputed
/*
void ifft(uint8_t len, complex double du[len], uint8_t stride, complex double
ret_du[len]) { 
	const float PI =  3.14159265358979323846;

	if (len == 1) {
		ret_du[0] = du[0];
		return;
	} else {
		complex double copy_du[len];
		memcpy(copy_du, du, sizeof(complex double) * len);
		complex double temp_ret_du[len];

		ifft(len/2, du, 2 * stride, ret_du);
		ifft(len/2, du + stride, 2 * stride, ret_du + len/2);

		complex double copy_du2[len];
		memcpy(copy_du2, ret_du, sizeof(complex double) * len);
		for (uint8_t i = 0; i < len/2; i++) {
			complex double temp_ret = ret_du[len/2 + i];
			complex double temp1 = cexp((I * PI * (float) (2*i) / (float) len)); 
			complex double temp2 = temp1 * temp_ret; 
			temp_ret_du[i] = ret_du[i] + temp2; 
			temp_ret_du[len/2 + i] = ret_du[i] - temp2;
		}
		for (uint8_t i = 0; i < len; i++) {
			ret_du[i] = temp_ret_du[i];
		}
	}
}
*/
