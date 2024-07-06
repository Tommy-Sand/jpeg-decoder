#include "huff_table.h"

#include <stdio.h>
#include <stdlib.h>

int new_huff_tables(Encoding process, HuffTables *hts) {
    if (hts == NULL) {
        return -1;
    }

    HuffTable *DC = hts->DCAC[0];
    HuffTable *AC = hts->DCAC[1];
    for (uint8_t i = 0; i < 4; i++) {
        for (uint8_t j = 0; j < 16; j++) {
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
            return -1;
    }
    return 0;
}

int32_t decode_huff_tables(uint8_t **encoded_data, HuffTables *hts) {
    if (encoded_data == NULL || *encoded_data == NULL || hts == NULL) {
        return -1;
    }
    uint8_t *ptr = *encoded_data;

    uint16_t len = (*(ptr++)) << 8;
    len += *(ptr++);

    if (len < (2 + 17)) {
        return -1;
    }

    uint8_t *end = ptr + len - 2;  // Subtract 2 for length already used
    while (ptr < end) {
        uint8_t class = ((*ptr) >> 4) & 0xF;
        uint8_t id = (*(ptr++)) & 0xF;
        if (id > hts->nDCAC) {
            return -1;
        }

        HuffTable *ht = hts->DCAC[class] + id;

        for (uint8_t i = 0; i < 16; i++) {
            ht->len[i] = (*(ptr++));
        }

        uint16_t code = 0;
        for (uint8_t i = 0; i < 16; i++) {
            uint8_t len = ht->len[i];
            uint8_t **symbols = ht->symbols + i;

            if (*symbols != NULL) {
                free(symbols);
            }

            if (len == 0) {
                code = code << 1;
                ht->min_codes[i] = -1;  //0xFFFF
                ht->max_codes[i] = -1;  //0xFFFF
                continue;
            }

            *symbols = (uint8_t *) malloc(len * sizeof(uint8_t));
            if (*symbols == NULL) {
                return -1;
            }

            for (uint8_t j = 0; j < len; j++) {
                (*symbols)[j] = *(ptr++);
            }
            ht->min_codes[i] = code;
            code += len;
            ht->max_codes[i] = code - 1;
            code = code << 1;
        }
    }

    if (0) {
        for (uint8_t i = 0; i < 2; i++) {
            if (i == 0) {
                printf("DC tables\n");
            } else {
                printf("AC tables\n");
            }
            for (uint8_t j = 0; j < hts->nDCAC; j++) {
                printf("\tj: %d\n", j);
                HuffTable *ht = hts->DCAC[i] + j;
                for (uint8_t k = 0; k < 16; k++) {
                    printf("len: %d\n", ht->len[k]);
                    if (ht->len[k] == 0) {
                        printf(
                            "\t\tk: %d, min_codes: %X, max_codes: %X, len: %d\n",
                            k + 1,
                            ht->min_codes[k],
                            ht->max_codes[k],
                            ht->len[k]
                        );
                        continue;
                    }
                    printf(
                        "\t\tk: %d, min_codes: %X, max_codes: %X, len: %d\n",
                        k + 1,
                        ht->min_codes[k],
                        ht->max_codes[k],
                        ht->len[k]
                    );
                    printf("\t\tsymbols: ");
                    for (uint8_t l = 0; l < ht->len[k]; l++) {
                        printf("%X, ", *(ht->symbols[k] + l));
                    }
                    printf("\n");
                }
            }
        }
    }

    *encoded_data = ptr;
    return 0;
}

int32_t free_huff_tables(HuffTables *hts) {
    if (hts == NULL) {
        return -1;
    }
    HuffTable *DC = hts->DCAC[0];
    HuffTable *AC = hts->DCAC[1];
    for (uint8_t i = 0; i < 4; i++) {
        for (uint8_t j = 0; j < 16; j++) {
            if ((DC + i)->symbols[j] == NULL) {
                free((DC + i)->symbols[j]);
            }
            if ((AC + i)->symbols[j] == NULL) {
                free((AC + i)->symbols[j]);
            }
        }
    }
    return 0;
}

void print_huff_tables(HuffTables *hts) {
    if (!hts) {
        return;
    }

    HuffTable *DCs = hts->DCAC[0];
    HuffTable *ACs = hts->DCAC[1];

    for (uint8_t i = 0; i < hts->nDCAC; i++) {
        printf("DC Table: %d\n", i);
        print_huff_table(DCs + i);
    }

    for (uint8_t i = 0; i < hts->nDCAC; i++) {
        printf("AC Table: %d\n", i);
        print_huff_table(ACs + i);
    }
}

void print_huff_table(HuffTable *ht) {
    if (!ht) {
        return;
    }

    for (uint8_t k = 0; k < 16; k++) {
        printf("len: %d\n", ht->len[k]);
        if (ht->len[k] == 0) {
            printf(
                "\t\tk: %d, min_codes: %X, max_codes: %X, len: %d\n",
                k + 1,
                ht->min_codes[k],
                ht->max_codes[k],
                ht->len[k]
            );
            continue;
        }
        printf(
            "\t\tk: %d, min_codes: %X, max_codes: %X, len: %d\n",
            k + 1,
            ht->min_codes[k],
            ht->max_codes[k],
            ht->len[k]
        );
        printf("\t\tsymbols: ");
        for (uint8_t l = 0; l < ht->len[k]; l++) {
            printf("%X, ", *(ht->symbols[k] + l));
        }
        printf("\n");
    }
}
