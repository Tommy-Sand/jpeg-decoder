#include "quant_table.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"

uint32_t log_du = 0;

/*
const uint8_t zigzag[64] = {
    0x00, 0x10, 0x01, 0x02, 0x11, 0x20, 0x30, 0x21, 
    0x12, 0x03, 0x04, 0x13, 0x22, 0x31, 0x40, 0x50, 
    0x41, 0x32, 0x23, 0x14, 0x05, 0x06, 0x15, 0x24, 
    0x33, 0x42, 0x51, 0x60, 0x70, 0x61, 0x52, 0x43, 
    0x34, 0x25, 0x16, 0x07, 0x17, 0x26, 0x35, 0x44, 
    0x53, 0x62, 0x71, 0x72, 0x63, 0x54, 0x45, 0x36, 
    0x27, 0x37, 0x46, 0x55, 0x64, 0x73, 0x74, 0x65, 
    0x56, 0x47, 0x57, 0x66, 0x75, 0x76, 0x67, 0x77
};
*/

const uint8_t zigzag[64] = {
    (0*8)+0, (0*8)+1, (1*8)+0, (2*8)+0, (1*8)+1, (0*8)+2, (0*8)+3, (1*8)+2, 
    (2*8)+1, (3*8)+0, (4*8)+0, (3*8)+1, (2*8)+2, (1*8)+3, (0*8)+4, (0*8)+5, 
    (1*8)+4, (2*8)+3, (3*8)+2, (4*8)+1, (5*8)+0, (6*8)+0, (5*8)+1, (4*8)+2, 
    (3*8)+3, (2*8)+4, (1*8)+5, (0*8)+6, (0*8)+7, (1*8)+6, (2*8)+5, (3*8)+4, 
    (4*8)+3, (5*8)+2, (6*8)+1, (7*8)+0, (7*8)+1, (6*8)+2, (5*8)+3, (4*8)+4, 
    (3*8)+5, (2*8)+6, (1*8)+7, (2*8)+7, (3*8)+6, (4*8)+5, (5*8)+4, (6*8)+3, 
    (7*8)+2, (7*8)+3, (6*8)+4, (5*8)+5, (4*8)+6, (3*8)+7, (4*8)+7, (5*8)+6, 
    (6*8)+5, (7*8)+4, (7*8)+5, (6*8)+6, (5*8)+7, (6*8)+7, (7*8)+6, (7*8)+7
};

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

int32_t
encode_quant_tables(QuantTables *qts, uint8_t **encoded_data, uint32_t len) {
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

int32_t decode_quant_table(Bitstream *bs, QuantTables *qts) {
    uint16_t len = next_byte(bs) << 8;
    len += next_byte(bs);
    //See Table B.4 in ITU-81
    if (len < 2 + 65) {
        return -1;
    }

    for (uint8_t i = 0; i < ((len - 2) / 65); i++) {
        uint8_t precision = (get_byte(bs) >> 4) & 0xF;
        uint8_t id = next_byte(bs) & 0xF;
        if (id > 3) {
            return -1;
        }
        QuantTable *qt = qts->tables + id;
        qt->precision = precision;

        for (uint8_t j = 0; j < 64; j++) {
            qt->table[zigzag[j]] = next_byte(bs);
            if (precision == 1) {
                qt->table[zigzag[j]] =
                    (qt->table[zigzag[j]] << 8) + next_byte(bs);
            }
        }

        if (DEBUG) {
            debug_print("Debugging Quant Table\n");
            debug_print("Precision: %dbits, Id: %d\n", 8 + (8 * precision), id);
            for (uint8_t i = 0; i < 64; i++) {
                debug_print(" %d ", qt->table[i]);
                if (((i + 1) % 8) == 0) {
                    debug_print("\n");
                }
            }
        }
    }

    return 0;
}

int32_t dequant_data_unit(QuantTable *qt, int16_t *du) {
    int16_t du_copy[64];
    memcpy(du_copy, du, sizeof(int16_t) * 64);

    for (uint8_t i = 0; i < 64; i++) {
        uint8_t temp = zigzag[i];
        du[temp] = du_copy[i] * qt->table[temp];
    }

    if (0) {
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
