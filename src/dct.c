#include <complex.h>
#define CLAMP(in) ((in > 255.0) ? 255 : ((in < 0.0) ? 0.0 : in))

void fast_2didct(int16_t du[64]);
void fast_idct(complex double in[8], complex double out[8]);

void fast_2didct(int16_t du[64]) {
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
        ifct(cdu + (i * 8), ret_dux + (i * 8));
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
        ifct(in_duy + (k * 8), ret_duy + (k * 8));
    }

    // transpose
    for (uint8_t j = 0; j < 8; j++) {
        du[j] = (int16_t) CLAMP((0.25 * creal(ret_duy[j * 8])) + 128.0);
        du[j + 8] =
            (int16_t) CLAMP((0.25 * creal(ret_duy[(j * 8) + 1])) + 128.0);
        du[j + 16] =
            (int16_t) CLAMP((0.25 * creal(ret_duy[(j * 8) + 2])) + 128.0);
        du[j + 24] =
            (int16_t) CLAMP((0.25 * creal(ret_duy[(j * 8) + 3])) + 128.0);
        du[j + 32] =
            (int16_t) CLAMP((0.25 * creal(ret_duy[(j * 8) + 4])) + 128.0);
        du[j + 40] =
            (int16_t) CLAMP((0.25 * creal(ret_duy[(j * 8) + 5])) + 128.0);
        du[j + 48] =
            (int16_t) CLAMP((0.25 * creal(ret_duy[(j * 8) + 6])) + 128.0);
        du[j + 56] =
            (int16_t) CLAMP((0.25 * creal(ret_duy[(j * 8) + 7])) + 128.0);
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
		temp_du[k] = 0.5 * cexp(I * PI * (float) k / 16.0) * (du[k] - I * du[8 -
k]);
	}

	complex double packed[4];
	for(uint8_t i = 0; i < 4; i++) {
		packed[i] = (0.5 * (temp_du[i] + temp_du[4 + i])) +
			 (0.5 * I * cexp (I * PI * 2.0 * i / 8.0) * (temp_du[i] - temp_du[4
+ i]));
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

void ifct(complex double in[8], complex double out[8]) {
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
ret_du[len]) { const float PI =  3.14159265358979323846;

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
			complex double temp1 = cexp((I * PI * (float) (2*i) / (float)
(len))); complex double temp2 = temp1 * temp_ret; temp_ret_du[i] = ret_du[i] +
temp2; temp_ret_du[len/2 + i] = ret_du[i] - temp2;
		}
		for (uint8_t i = 0; i < len; i++) {
			ret_du[i] = temp_ret_du[i];
		}
	}
}
*/
