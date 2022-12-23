#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <cassert>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <algorithm>

class jpeg_image{
public:
    jpeg_image();
    jpeg_image(const char* path);
    jpeg_image(void *data, uint64_t size);
    decode_header;
    decode_quant_table;
    decode_comments;
    decode_huffman;
    decode_image;

private:
    find_markers();

    std::filesystem::path p;
    std::ifstream image;
    uint8_t *data;
    uint64_t size;
    Header header;
    Huffman_table huff_table;
    DCT_header dct_header;
    Quant_table quant_table;
    Scan_header scan_header;
};

class Header{
private:
    uint8_t *data;
    uint32_t size;

}

class JFIF_header: public Header{
public:
    JFIF_header(uint8_t *data): data{data} {}

private:
    uint8_t *data;
    uint16_t length;
    uint64_t identfier;
    uint16_t JFIF_version;
    uint8_t dens_units;
    uint16_t xdens;
    uint16_t ydens;
    uint8_t xthumbnail;
    uint8_t ythumbnail;
    std::vector<std::vector<std::vector<uint8_t>>> thumb;
};

class Huffman_table{
public:
    huffman_table(uin8_t* data);
    void parse_data();
    uint32_t decode_coefficient();

private:
    uint8_t* data;
    uint16_t length;
    uint8_t type;
    uint8_t table_ID;
    int16_t *min_symbol;
    int16_t *max_symbol;
    std::vector<uint8_t> *symbol_array;
};

class DCT_header{
public:
    DCTheader(uint8_t *data): data{data} {}

private:
    uint8_t *data;
    uint16_t length;
    uint8_t sample_percision;
    uint16_t height;
    uint16_t width;
    uint8_t num_chans;
    Channel_info *chan_infos;
};

class Quant_table{
public:
    quant_table(uint8_t *data): data{data} {}

private:
    uint8_t *data;
    uint16_t length;
    uint8_t size;
    uint8_t id;
    uint16_t **quant_table;
};

class Scan_header{
public:
	Scan_header(uint8_t *data): data{data} {}

private:
    uint8_t *data;
    uint16_t length;
    uint8_t num_chans;
    struct Chan_specifier *chan_specifiers;
    uint8_t spectral_start;
    uint8_t spectral_end;
    uint8_t successive_approx;
};

//change to struct
struct Chan_specifier{
    uint8_t componentID;
    uint8_t Huffman_DC;
    uint8_t Huffman_AC;
};

class Channel_info{
public:
    uint8_t identifier;
    uint8_t horz_sampling;
    uint8_t vert_sampling;
    uint8_t qtableID;
};