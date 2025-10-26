#include <complex.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "dct.c"

int main() {
    const float PI = 3.14159265358979323846;
    complex double in[8] = {1.0, -1.0, 1.0, -1.0, 5.0, -4.0, -3.0, 2.0};
    complex double out1[8] = {0.0};
    printf("wrong value, faster dct\n");
    fast_dct(in, out1);

    printf("results: \n");
    for (uint8_t i = 0; i < 8; i++) {
        printf("%f + %fi \n", creal(out1[i]), cimag(out1[i]));
    }
    printf("\n");

    printf("correct value, fast dct\n");
    complex double out2[8] = {0.0};
    correct_fast_dct(in, out2);

    printf("results: \n");
    for (uint8_t i = 0; i < 8; i++) {
        printf("%f + %fi \n", creal(out2[i]), cimag(out2[i]));
    }
    printf("\n");

    printf("correct_value, slow dct\n");
    printf("results: \n");
    for (uint8_t i = 0; i < 8; i++) {
        double out1 = 0.0;
        for (uint8_t k = 0; k < 8; k++) {
            out1 += 2 * in[k] * cos(PI * (2.0 * k + 1.0) * i / 16.0);
        }
        printf("%f\n", out1);
    }
    printf("\n");
}
