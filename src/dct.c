#include "dct.h"
#include <string.h>
#include <stdio.h>

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
    
	//transpose TODO Remove clamp and find to keep the precision since 
	// it is required for the quatization stage https://en.wikipedia.org/wiki/JPEG#Discrete_cosine_transform
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
	const float PI =  3.14159265358979323846;

	complex double temp_du[8];
	for (uint8_t i = 0; i < 4; i++) {
		temp_du[i] = in[2*i];
		temp_du[4 + i] = in[16 - 2*(4 + i) - 1];
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
	const float PI =  3.14159265358979323846;

	complex double temp_du[8];
	for (uint8_t i = 0; i < 4; i++) {
		temp_du[i] = in[2*i];
		temp_du[4 + i] = in[16 - 2*(4 + i) - 1];
	}

	complex double packed[4];
	for(uint8_t i = 0; i < 4; i++) {
		packed[i] = temp_du[2*i] + I * temp_du[2*i + 1];
	}

	complex double fft_ret_du[4];
	fft(4, packed, 1, fft_ret_du);

	complex double unpacked_du[8];
	for(uint8_t i = 1; i < 3; i++) {
		complex double temp1 = 0.5 * (fft_ret_du[i] + conj(fft_ret_du[4 - i]));
		complex double temp2 = 0.5 * I * cexp(I * -2.0 * i * PI / 8) * (fft_ret_du[i] - conj(fft_ret_du[4 - i]));
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

void fft(uint8_t len, complex double du[len], uint8_t stride, complex double ret_du[len]) {
	const float PI =  3.14159265358979323846;

	if (len == 1) {
		ret_du[0] = du[0];
		return;
	} else {
		complex double copy_du[len];
		memcpy(copy_du, du, sizeof(complex double) * len);
		complex double temp_ret_du[len];

		fft(len/2, du, 2 * stride, ret_du);
		fft(len/2, du + stride, 2 * stride, ret_du + len/2);

		complex double copy_du2[len];
		memcpy(copy_du2, ret_du, sizeof(complex double) * len);
		for (uint8_t i = 0; i < len/2; i++) {
			complex double temp_ret = ret_du[len/2 + i];
			complex double temp1 = cexp((I * PI * (float) (-2*i) / (float) len)); 
			complex double temp2 = temp1 * temp_ret; 
			temp_ret_du[i] = ret_du[i] + temp2; 
			temp_ret_du[len/2 + i] = ret_du[i] - temp2;
		}
		for (uint8_t i = 0; i < len; i++) {
			ret_du[i] = temp_ret_du[i];
		}
	}
}

//precision: 0 indicates an 8 bit precision level, add 128
//precision: 1 indicates a 12 bit precision level, add 2048
void fast_2didct(int16_t du[64], uint8_t precision) {
    complex double cdu[64] = {0.0};
    for (uint8_t i = 0; i < 64; i++) {
        cdu[i] = (complex double) du[i];
    }
    for (uint8_t i = 0; i < 8; i++) {
        cdu[i] *= 0.707106781;
        cdu[i * 8] *= 0.707106781;
    }

    complex double ret_dux[64] = {0};
    for (uint8_t i = 0; i < 8; i++) {
        fast_idct(cdu + (i * 8), ret_dux + (i * 8));
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
        fast_idct(in_duy + (k * 8), ret_duy + (k * 8));
    }

    // transpose
	double level_shift = 128.0;
	if (!precision) {
		for (uint8_t j = 0; j < 8; j++) {
			du[j] = (int16_t) CLAMP_8((0.25 * creal(ret_duy[j * 8])) + level_shift);
			du[j + 8] =
				(int16_t) CLAMP_8((0.25 * creal(ret_duy[(j * 8) + 1])) + level_shift);
			du[j + 16] =
				(int16_t) CLAMP_8((0.25 * creal(ret_duy[(j * 8) + 2])) + level_shift);
			du[j + 24] =
				(int16_t) CLAMP_8((0.25 * creal(ret_duy[(j * 8) + 3])) + level_shift);
			du[j + 32] =
				(int16_t) CLAMP_8((0.25 * creal(ret_duy[(j * 8) + 4])) + level_shift);
			du[j + 40] =
				(int16_t) CLAMP_8((0.25 * creal(ret_duy[(j * 8) + 5])) + level_shift);
			du[j + 48] =
				(int16_t) CLAMP_8((0.25 * creal(ret_duy[(j * 8) + 6])) + level_shift);
			du[j + 56] =
				(int16_t) CLAMP_8((0.25 * creal(ret_duy[(j * 8) + 7])) + level_shift);
		}
	} else {
		double level_shift = 2048.0;
		for (uint8_t j = 0; j < 8; j++) {
			du[j] = (int16_t) CLAMP_16((0.25 * creal(ret_duy[j * 8])) + level_shift);
			du[j + 8] =
				(int16_t) CLAMP_16((0.25 * creal(ret_duy[(j * 8) + 1])) + level_shift);
			du[j + 16] =
				(int16_t) CLAMP_16((0.25 * creal(ret_duy[(j * 8) + 2])) + level_shift);
			du[j + 24] =
				(int16_t) CLAMP_16((0.25 * creal(ret_duy[(j * 8) + 3])) + level_shift);
			du[j + 32] =
				(int16_t) CLAMP_16((0.25 * creal(ret_duy[(j * 8) + 4])) + level_shift);
			du[j + 40] =
				(int16_t) CLAMP_16((0.25 * creal(ret_duy[(j * 8) + 5])) + level_shift);
			du[j + 48] =
				(int16_t) CLAMP_16((0.25 * creal(ret_duy[(j * 8) + 6])) + level_shift);
			du[j + 56] =
				(int16_t) CLAMP_16((0.25 * creal(ret_duy[(j * 8) + 7])) + level_shift);
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

inline void fast_idct(complex double in[8], complex double out[8]) {
    complex double pass1[8] = {
        in[0],
        0.5 * (0.9807853 + 0.1950903 * I) * (in[1] - I * in[8 - 1]),
        0.5 * (0.9238800 + 0.3826830 * I) * (in[2] - I * in[8 - 2]),
        0.5 * (0.8314700 + 0.5555700 * I) * (in[3] - I * in[8 - 3]),
        0.5 * (0.7071070 + 0.7071070 * I) * (in[4] - I * in[8 - 4]),
        0.5 * (0.5555700 + 0.8314700 * I) * (in[5] - I * in[8 - 5]),
        0.5 * (0.3826830 + 0.9238800 * I) * (in[6] - I * in[8 - 6]),
        0.5 * (0.1950900 + 0.9807850 * I) * (in[7] - I * in[8 - 7])};

    complex double pass2[4] = {
        (0.5 * (pass1[0] + pass1[4])) + (0.5 * I * (pass1[0] - pass1[4])),
        (0.5 * (pass1[1] + pass1[5]))
            + (0.5 * I * (0.707107 + 0.707107 * I) * (pass1[1] - pass1[5])),
        (0.5 * (pass1[2] + pass1[6])) + (0.5 * I * I * (pass1[2] - pass1[6])),
        (0.5 * (pass1[3] + pass1[7]))
            + (0.5 * I * (-0.707107 + 0.707107 * I) * (pass1[3] - pass1[7]))};

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

    complex double pass4[8] = {
        2 * creal(pass3[0]),
        2 * cimag(pass3[0]),
        2 * creal(pass3[1]),
        2 * cimag(pass3[1]),
        2 * creal(pass3[2]),
        2 * cimag(pass3[2]),
        2 * creal(pass3[3]),
        2 * cimag(pass3[3])};

    out[0] = pass4[0];
    out[1] = pass4[7];
    out[2] = pass4[1];
    out[3] = pass4[6];
    out[4] = pass4[2];
    out[5] = pass4[5];
    out[6] = pass4[3];
    out[7] = pass4[4];
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
