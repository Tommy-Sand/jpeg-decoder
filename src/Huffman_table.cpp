#include "jpeg_decoder.hh"

Huffman_table::Huffman_table(uint8_t *data): data{data} {
    uint16_t pos = 1;

    this->length = *(data + (++pos)) << 8;
    this->length += *(data + (++pos));

    this->type = *(data + (++pos)) >> 4;
    this->table_id = *(data + pos) & 0xF;

    this->num_codes_len_i = new uint8_t[16];
    for(int i = 0; i < 16; i++)
        this->num_codes_len_i[i] = *(data + (++pos));

    this->min_code_value = new uint8_t[16];
    this->max_code_value = new uint8_t[16];
    this->symbol_array = new uint8_t*[16];
    for(int i = 0; i < 16; i++){
        uint8_t num_codes = this->num_codes_len_i[i];
        this->symbol_array[i] = new uint8_t[num_codes];

        for(int j = 0; j < num_codes; j++)
            this->symbol_array[i][j] = *(data + (++pos));

        this->min_code_value[i] = this->symbol_array[i][0];
        this->max_code_value[i] = this->symbol_array[i][num_codes-1];
    }

}