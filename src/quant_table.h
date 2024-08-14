#pragma once
#include <stdint.h>

typedef struct {
    uint8_t precision;
    uint16_t table[64];
} QuantTable;

typedef struct {
    QuantTable tables[4];
} QuantTables;

QuantTables *new_quant_tables();

int32_t encode_quant_tables_len(QuantTables *qts);

int32_t encode_quant_tables(QuantTables *qts, uint8_t **encoded_data, uint32_t len); 

int32_t decode_quant_table(uint8_t **encoded_data, QuantTables *qts);

int32_t dequant_data_unit(QuantTable *qt, int16_t *du);

int32_t free_quant_tables(QuantTables *qts);

int32_t free_quant_table(QuantTable *qt);
