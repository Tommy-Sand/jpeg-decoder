#include "quant_table.h"
#include "debug.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint32_t log_du = 0;

const uint8_t zigzag[64] = {
    0x00, 0x10, 0x01, 0x02, 0x11, 0x20, 0x30, 0x21, 0x12, 0x03, 0x04,
    0x13, 0x22, 0x31, 0x40, 0x50, 0x41, 0x32, 0x23, 0x14, 0x05, 0x06,
    0x15, 0x24, 0x33, 0x42, 0x51, 0x60, 0x70, 0x61, 0x52, 0x43, 0x34,
    0x25, 0x16, 0x07, 0x17, 0x26, 0x35, 0x44, 0x53, 0x62, 0x71, 0x72,
    0x63, 0x54, 0x45, 0x36, 0x27, 0x37, 0x46, 0x55, 0x64, 0x73, 0x74,
    0x65, 0x56, 0x47, 0x57, 0x66, 0x75, 0x76, 0x67, 0x77};

int32_t encode_quant_tables_len(QuantTables *qts) {
	if (qts == NULL) {
		return -1; 
	}

	uint16_t lq = 2;
	for (uint8_t i = 0; i < 4; i++) {
		lq += 65 + (64 * qts->tables[i].precision);
	}
	return lq;
}

int32_t encode_quant_tables(QuantTables *qts, uint8_t **encoded_data, uint32_t len) {
    if (qts == NULL
        || (encoded_data == NULL
            || (encoded_data != NULL && *encoded_data != NULL))) {
        return -1;
    }

	int32_t lq = encode_quant_tables_len(qts);
	if (lq == -1) {
		return -1;
	}
	if (encoded_data != NULL && (int64_t) lq <= (int64_t) len) {
		return -1;
	} else if (encoded_data == NULL) {
		uint8_t *allocated_data = malloc(len);
		if (allocated_data == NULL) {
			return -1;
		}
		*encoded_data = allocated_data;
	}

    uint32_t idx = 0;
    (*encoded_data)[idx++] = (uint8_t) ((uint16_t) lq) >> 8;
    (*encoded_data)[idx++] = (uint8_t) ((uint16_t) lq) && 0xFF;

    for (uint8_t i = 0; i < 4; i++) {
		QuantTable qt = qts->tables[i];
		(*encoded_data)[idx++] = (qt.precision << 4) & i;
		if (qt.precision) {
			for (uint8_t k = 0; k < 64; k++) {
				(*encoded_data)[idx++] = qt.table[k];
			}
		} else {
			for (uint8_t k = 0; k < 64; k++) {
				(*encoded_data)[idx++] = (uint8_t) (qt.table[k] >> 8);
				(*encoded_data)[idx++] = (uint8_t) (qt.table[k] & 0xFF);
			}
		}
	}
    return 0;
}

int32_t decode_quant_table(uint8_t **encoded_data, QuantTables *qts) {
    if (encoded_data == NULL || *encoded_data == NULL || qts == NULL) {
        return -1;
    }
    uint8_t *ptr = *encoded_data;

    uint16_t len = (*(ptr++)) << 8;
    len += *(ptr++);
    //See Table B.4 in ITU-81
    if (len < 2 + 65) {
        return -1;
    }

    uint8_t *end = ptr + len - 2;  //Subtract 2 for length already read
    while (ptr < end) {
        uint8_t precision = ((*ptr) >> 4) & 0xF;
        uint8_t id = (*(ptr++)) & 0xF;
        if (id > 3) {
            return -1;
        }
        QuantTable *qt = qts->tables + id;
        qt->precision = precision;

        for (uint8_t i = 0; i < 64; i++) {
            uint8_t row = zigzag[i] & 0xF;
            uint8_t col = (zigzag[i] >> 4) & 0xF;
            qt->table[(row * 8) + col] = *(ptr++);
            if (precision == 1) {
                qt->table[(row * 8) + col] =
                    (qt->table[(row * 8) + col] << 8) + *(ptr++);
            }
        }

        if (DEBUG) {
            debug_print("Debugging Quant Table\n");
            debug_print("Precision: %dbits, Id: %d\n", 8 + (8 * precision), id);
            for (uint8_t i = 0; i < 64; i++) {
                printf(" %d ", qt->table[i]);
                if (((i + 1) % 8) == 0) {
                    printf("\n");
                }
            }
        }
    }

    *encoded_data = ptr;
    return 0;
}

int32_t dequant_data_unit(QuantTable *qt, int16_t *du) {
    int16_t du_copy[64];
    memcpy(du_copy, du, sizeof(int16_t) * 64);

    for (uint8_t i = 0; i < 64; i++) {
        uint8_t row = zigzag[i] & 0xF;
        uint8_t col = (zigzag[i] >> 4) & 0xF;
        du[(row * 8) + col] = du_copy[i] * qt->table[(row * 8) + col];
    }

    if (DEBUG) {
        debug_print("Dequant data block data unit: %d\n", log_du);
        for (uint8_t i = 0; i < 64; i++) {
            if (i % 8 == 0) {
                printf("\n");
            }
            printf("%d, ", du[i]);
        }
        printf("\n");
    }
    log_du++;
    return 0;
}
