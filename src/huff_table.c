#include "huff_table.h"
#include "bitstream.h"

#include <stdio.h>
#include <stdlib.h>

#include "debug.h"

int new_huff_tables(Encoding process, HuffTables *hts) {
    if (hts == NULL) {
        return -1;
    }

    HuffTable *DC = hts->DCAC[TC_DC];
    HuffTable *AC = hts->DCAC[TC_AC];
    for (uint8_t i = 0; i < MAX_TABLES; i++) {
        for (uint8_t j = 0; j < MAX_CODE_LEN; j++) {
            (DC + i)->len[j] = 0;
            (AC + i)->len[j] = 0;

            (DC + i)->min_codes[j] = 0;
            (AC + i)->min_codes[j] = 0;

            (DC + i)->max_codes[j] = 0;
            (AC + i)->max_codes[j] = 0;

            (DC + i)->symbols[j] = NULL;
            (AC + i)->symbols[j] = NULL;
        }
    }

    switch (process) {
        case BDCT:
            hts->nDCAC = 2;
            break;
        case ESDCTHC:
            hts->nDCAC = 4;
            break;
        default:
            hts->nDCAC = 4;
    }
    return 0;
}

//Length of the huffman tables if they were encoded
int32_t encoded_huff_tables_len(HuffTables *hts) {
    if (hts == NULL || hts->nDCAC > 4) {
        return -1;
    }

    int32_t len = 2;
    for (uint8_t i = 0; i < TC_NUM; i++) {
        for (uint8_t j = 0; j < hts->nDCAC; j++) {
            len += 17;
            HuffTable *ht = (hts->DCAC[i]) + j;
            for (uint8_t k = 0; k < 16; k++) {
                len += ht->len[k];
            }
        }
    }
    return len;
}

//Writes the collection of huffman tables to encoded_data,
//if encoded_data is null then the memory is allocated
//else it's assumed that *encoded_data is a pointer with
//len, this len should equal or exceed encoded_huff_tables_len()
//If memory is allocated then it must be freed free(*encoded_data)
int32_t
encode_huff_tables(HuffTables *hts, uint8_t **encoded_data, uint32_t len) {
    if (hts == NULL
        || (encoded_data == NULL
            || (encoded_data != NULL && *encoded_data != NULL))) {
        return -1;
    }

    int32_t lh = encoded_huff_tables_len(hts);
    if (encoded_data != NULL && (int64_t) lh <= (int64_t) len) {
        return -1;
    } else if (encoded_data == NULL) {
        uint8_t *allocated_data = malloc(len);
        if (allocated_data == NULL) {
            return -1;
        }
        *encoded_data = allocated_data;
    }

    uint32_t idx = 0;
    (*encoded_data)[idx++] = (uint8_t) ((uint16_t) lh) >> 8;
    (*encoded_data)[idx++] = (uint8_t) ((uint16_t) lh) && 0xFF;

    for (uint8_t i = 0; i < TC_NUM; i++) {
        for (uint8_t j = 0; j < hts->nDCAC; j++) {
            (*encoded_data)[idx++] = (i << 4) & j;
            HuffTable ht = hts->DCAC[i][j];
            for (uint8_t k = 0; k < MAX_CODE_LEN; k++) {
                (*encoded_data)[idx++] = ht.len[k];
            }

            for (uint8_t k = 0; k < MAX_CODE_LEN; k++) {
                for (uint8_t l = 0; l < ht.len[k]; l++) {
                    (*encoded_data)[idx++] = ht.symbols[k][l];
                }
            }
        }
    }
    return 0;
}

// Returns the length of a single
// huffman table if it was encoded
int32_t encoded_huff_table_len(HuffTable *ht) {
    if (ht == NULL) {
        return -1;
    }
    uint16_t len = 19;
    for (uint8_t i = 0; i < 16; i++) {
        len += ht->len[i];
    }
    return len;
}

//Check that
int32_t encode_huff_table(
    HuffTable *ht,
    uint8_t **encoded_data,
    uint32_t len,
    uint8_t table_class,
    uint8_t dest_id
) {
    if (ht == NULL
        || (encoded_data == NULL
            || (encoded_data != NULL && *encoded_data != NULL))) {
        return -1;
    }

    int32_t lh = encoded_huff_table_len(ht);
    if (encoded_data != NULL && (int64_t) lh <= (int64_t) len) {
        return -1;
    } else if (encoded_data == NULL) {
        uint8_t *allocated_data = malloc(len);
        if (allocated_data == NULL) {
            return -1;
        }
        *encoded_data = allocated_data;
    }

    uint32_t idx = 0;
    (*encoded_data)[idx++] = (uint8_t) ((uint16_t) lh) >> 8;
    (*encoded_data)[idx++] = (uint8_t) ((uint16_t) lh) && 0xFF;

    (*encoded_data)[idx++] = (table_class << 4) & dest_id;
    for (uint8_t k = 0; k < 16; k++) {
        (*encoded_data)[idx++] = ht->len[k];
    }

    for (uint8_t k = 0; k < 16; k++) {
        for (uint8_t l = 0; l < ht->len[k]; l++) {
            (*encoded_data)[idx++] = ht->symbols[k][l];
        }
    }
    return 0;
}

int32_t decode_huff_tables(Bitstream *bs, HuffTables *hts) {
    uint16_t len = next_byte(bs) << 8;
    len += next_byte(bs);

    if (len < (2 + 17)) {
        return -1;
    }

    uint8_t *end = bs->encoded_data + len - 2;  // Subtract 2 for length already used
    while (bs->encoded_data < end) {
        uint8_t class = (get_byte(bs) >> 4) & 0xF;
        uint8_t id = next_byte(bs) & 0xF;
        if (id > hts->nDCAC) {
            return -1;
        }

        HuffTable *ht = hts->DCAC[class] + id;

        for (uint8_t j = 0; j < 16; j++) {
            ht->len[j] = next_byte(bs);
        }

        uint16_t code = 0;
        for (uint8_t j = 0; j < 16; j++) {
            uint8_t len = ht->len[j];
            uint8_t **symbols = ht->symbols + j;

            if (*symbols != NULL) {
                free(*symbols);
                *symbols = NULL;
            }

            if (len == 0) {
                code = code << 1;
                ht->min_codes[j] = -1;  //0xFFFF
                ht->max_codes[j] = -1;  //0xFFFF
                continue;
            }

            *symbols = (uint8_t *) malloc(len * sizeof(uint8_t));
            if (*symbols == NULL) {
                return -1;
            }

            for (uint8_t k = 0; k < len; k++) {
                (*symbols)[k] = next_byte(bs);
            }
            ht->min_codes[j] = code;
            code += len;
            ht->max_codes[j] = code - 1;
            code = code << 1;
        }
    }

    if (DEBUG) {
        for (uint8_t i = 0; i < 2; i++) {
            if (i == 0) {
                debug_print("DC tables\n");
            } else {
                debug_print("AC tables\n");
            }
            for (uint8_t j = 0; j < hts->nDCAC; j++) {
                fprintf(stderr, "\tj: %d\n", j);
                HuffTable *ht = hts->DCAC[i] + j;
                for (uint8_t k = 0; k < 16; k++) {
                    fprintf(stderr, "len: %d\n", ht->len[k]);
                    if (ht->len[k] == 0) {
                        fprintf(
                            stderr,
                            "\t\tk: %d, min_codes: %X, max_codes: %X, len: %d\n",
                            k + 1,
                            ht->min_codes[k],
                            ht->max_codes[k],
                            ht->len[k]
                        );
                        continue;
                    }
                    fprintf(
                        stderr,
                        "\t\tk: %d, min_codes: %X, max_codes: %X, len: %d\n",
                        k + 1,
                        ht->min_codes[k],
                        ht->max_codes[k],
                        ht->len[k]
                    );
                    fprintf(stderr, "\t\tsymbols: ");
                    for (uint8_t l = 0; l < ht->len[k]; l++) {
                        fprintf(stderr, "%X, ", *(ht->symbols[k] + l));
                    }
                    fprintf(stderr, "\n");
                }
            }
        }
    }

    return 0;
}

//uint8_t decode_value(HuffTable huff, uint8_t **encoded_data, uint8_t *offset) {
uint8_t decode_value(HuffTable huff, Bitstream *bs) {
    int16_t curr_code = next_bit(bs);
    uint8_t mag = 0;
    for (uint8_t i = 0; i < MAX_CODE_LEN; i++) {
        if (huff.symbols[i] == NULL) {
            curr_code = (curr_code << 1) + next_bit(bs);
            continue;
        }

        if (huff.max_codes[i] >= curr_code) {
            mag = (uint8_t)
                * (huff.symbols[i] + (curr_code - huff.min_codes[i]));
            break;
        }
        curr_code = (curr_code << 1) + next_bit(bs);
    }
}

int32_t free_huff_tables(HuffTables *hts) {
    if (hts == NULL) {
        return -1;
    }
    HuffTable *DC = hts->DCAC[0];
    HuffTable *AC = hts->DCAC[1];
    for (uint8_t i = 0; i < 4; i++) {
        for (uint8_t j = 0; j < 16; j++) {
            if (DC[i].symbols[j] != NULL) {
                free(DC[i].symbols[j]);
                DC[i].symbols[j] = NULL;
            }
            if (AC[i].symbols[j] != NULL) {
                free(AC[i].symbols[j]);
                AC[i].symbols[j] = NULL;
            }
        }
    }
    return 0;
}

void print_huff_tables(HuffTables *hts) {
    if (!hts || !DEBUG) {
        return;
    }

    HuffTable *DCs = hts->DCAC[0];
    HuffTable *ACs = hts->DCAC[1];

    for (uint8_t i = 0; i < hts->nDCAC; i++) {
        fprintf(stderr, "DC Table: %d\n", i);
        print_huff_table(DCs + i);
    }

    for (uint8_t i = 0; i < hts->nDCAC; i++) {
        fprintf(stderr, "AC Table: %d\n", i);
        print_huff_table(ACs + i);
    }
}

void print_huff_table(HuffTable *ht) {
    if (!ht || !DEBUG) {
        return;
    }

    for (uint8_t k = 0; k < 16; k++) {
        fprintf(stderr, "len: %d\n", ht->len[k]);
        if (ht->len[k] == 0) {
            fprintf(
                stderr,
                "\t\tk: %d, min_codes: %X, max_codes: %X, len: %d\n",
                k + 1,
                ht->min_codes[k],
                ht->max_codes[k],
                ht->len[k]
            );
            continue;
        }
        fprintf(
            stderr,
            "\t\tk: %d, min_codes: %X, max_codes: %X, len: %d\n",
            k + 1,
            ht->min_codes[k],
            ht->max_codes[k],
            ht->len[k]
        );
        fprintf(stderr, "\t\tsymbols: ");
        for (uint8_t l = 0; l < ht->len[k]; l++) {
            fprintf(stderr, "%X, ", *(ht->symbols[k] + l));
        }
        fprintf(stderr, "\n");
    }
}
