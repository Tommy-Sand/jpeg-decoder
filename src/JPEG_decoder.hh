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

enum Encoding: uint8_t;

//change to struct
struct Chan_specifier{
    uint8_t componentID;
    uint8_t Huffman_DC;
    uint8_t Huffman_AC;
};

struct Channel_info{
    uint8_t id;
    uint8_t horz_sampling;
    uint8_t vert_sampling;
    uint8_t qtableID;
};

class Header{
private:
    uint8_t *data;
    uint32_t size;

};

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
    Huffman_table(uint8_t* data);
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

class Frame_header{
public:
    Frame_header(uint8_t *data);
    uint8_t *get_data() {return this->data;}
    uint16_t get_length() {return this->length;}
    uint8_t get_precision() {return this->precision;}
    uint16_t get_height() {return this->height;}
    uint16_t get_width() {return this->width;}
    uint8_t get_num_chans() {return this->num_chans;}
    struct Channel_info *get_chan_info(uint8_t index) { 
        if(index < this->num_chans) return this->chan_infos + index;
        else return nullptr;
    }

private:
    enum Encoding encoding_process;
    uint8_t *data;
    uint16_t length;
    uint8_t precision;
    uint16_t height;
    uint16_t width;
    uint8_t num_chans;
    struct Channel_info *chan_infos;

};

class Quant_table{
public:
    Quant_table(uint8_t *data): data{data} {}

private:
    uint8_t *data;
    uint16_t length;
    uint8_t size;
    uint8_t id;
    uint16_t **quant_table;
};

class Scan_header{
public:
	Scan_header(uint8_t *data);
    uint8_t *get_data() {return this->data;}
    uint16_t get_length() {return this->length;}
    uint8_t get_num_chans() {return this->num_chans;}
    struct Chan_specifier *get_chan_spec(uint8_t index) { 
        if(index < this->num_chans) return this->chan_specifiers + index;
        else return nullptr;
    }
    uint16_t get_spectral_start() {return this->spectral_start;}
    uint16_t get_spectral_end() {return this->spectral_end;}
    uint16_t get_prev_approx() {return this->prev_approx;}
    uint16_t get_succ_approx() {return this->succ_approx;}
private:
    uint8_t *data;
    uint16_t length;
    uint8_t num_chans;
    struct Chan_specifier *chan_specifiers;
    uint8_t spectral_start;
    uint8_t spectral_end;
    uint8_t prev_approx;
    uint8_t succ_approx;
};

class jpeg_image{
public:
    jpeg_image();
    jpeg_image(const char* path);
    jpeg_image(void *data, uint64_t size);
    void decode_header();//TBD
    void decode_quant_table();//TBD
    void decode_comments();//TDB
    void decode_huffman();//TBD
    void decode_image();//TBD

private:
    void find_markers();

    std::filesystem::path p;
    std::ifstream image;
    uint8_t *data;
    uint64_t size = 0;
    Frame_header* frame_header;
    /*
    Header header;
    Huffman_table huff_table;
    DCT_header dct_header;
    Quant_table quant_table;
    Scan_header scan_header;
    */
};