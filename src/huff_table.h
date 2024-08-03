#pragma once
#include <stdint.h>

#include "frame_header.h"

typedef struct {
    int16_t min_codes[16];
    int16_t max_codes[16];
    uint8_t len[16];
    uint8_t *(symbols[16]);
} HuffTable;

typedef struct {
    uint8_t nDCAC;  //num DC tables which is also the  num AC tables
    HuffTable DCAC[2][4];  //[0][] DC tables [1][] AC tables
} HuffTables;

int new_huff_tables(Encoding process, HuffTables *hts);

int32_t decode_huff_tables(uint8_t **encoded_data, HuffTables *hts);

//Length of the huffman tables if they were encoded
int32_t encoded_huff_tables_len(HuffTables *hts);

int32_t
encode_huff_tables(HuffTables *hts, uint8_t **encoded_data, uint32_t len);

//Length of the huffman table if it was encoded
int32_t encoded_huff_table_len(HuffTable *ht);

int32_t encode_huff_table(
    HuffTable *ht,
    uint8_t **encoded_data,
    uint32_t len,
    uint8_t table_class,
    uint8_t dest_id
);

int32_t free_huff_tables(HuffTables *hts);

void print_huff_tables(HuffTables *hts);

void print_huff_table(HuffTable *ht);
