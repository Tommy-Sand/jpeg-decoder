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
extern uint8_t zigzag[64]; 

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

struct Component{
	uint32_t num_data_blocks;
	int16_t (*data_blocks)[8][8];
};

struct Image_block{
	struct Component *components; 
};

struct size{
	uint16_t X;
	uint16_t Y;
};

struct RGB{
	uint8_t R;
	uint8_t G;
	uint8_t B;
};

struct RGB_block{
	struct RGB rgb_block[8][8];
};

struct YCBCR{
	int16_t Y;
	int16_t CB;
	int16_t CR;
};

class Comment{
public:
	Comment() {};
    Comment(uint8_t **data);
private:
    uint16_t length;
    std::string comment;
};

class APP_header{
public:
    APP_header(uint8_t **data);
private:
    uint8_t *data;
    uint16_t length;
};

class JFIF_header: public APP_header{
public:
    JFIF_header(uint8_t **data);
	uint16_t get_version() {return this->version;};
	uint8_t get_dens_units() {return this->dens_units;};
	uint16_t get_x_dens() {return this->x_dens;};
	uint16_t get_y_dens() {return this->y_dens;};
	uint8_t get_x_thumbnail() {return this->x_thumbnail;};
	uint8_t get_y_thumbnail() {return this->y_thumbnail;};
	uint16_t get_thumbnail_length() {return this->thumbnail_length;};
	uint8_t *get_thumbnail() {return this->thumbnail;};

private:
    uint16_t version;
    uint8_t dens_units;
    uint16_t x_dens;
    uint16_t y_dens;
    uint8_t x_thumbnail;
    uint8_t y_thumbnail;
    uint16_t thumbnail_length;
    uint8_t *thumbnail;
};

class Restart_interval{
public:
	Restart_interval() {};
    Restart_interval(uint8_t **data);
	uint16_t get_num_MCUs() {return this->num_MCUs;};
private:
    uint16_t num_MCUs; 
};

class Huffman_table{
public:
	Huffman_table() {};
    Huffman_table(uint8_t** data);
	uint16_t get_length() {return this->length;};
	uint8_t get_type() {return this->type;};
	uint8_t get_table_id() {return this->table_id;};
	uint8_t *get_num_codes_len_i() {return this->num_codes_len_i;};
	int32_t *get_min_code_values() {return this->min_code_value;};
	int32_t *get_max_code_values() {return this->max_code_value;};
	uint8_t **get_symbol_arrays() {return this->symbol_array;};

private:
	uint16_t length;
    uint8_t type;
    uint8_t table_id;
    uint8_t *num_codes_len_i;
    int32_t *min_code_value;
    int32_t *max_code_value;
    uint8_t **symbol_array;
};

class Frame_header{
public:
	Frame_header() {};
    Frame_header(uint8_t **data);
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
    uint16_t length;
    uint8_t precision;
    uint16_t height;
    uint16_t width;
    uint8_t num_chans;
    struct Channel_info *chan_infos;
};

class Quantization_table{
public:
	Quantization_table() {};
    Quantization_table(uint8_t **data);
    uint16_t get_length() {return length;}
    uint8_t get_precision() {return percision;}
    uint8_t get_id() {return id;}
    uint16_t **get_quantization_table() {return quant_table;}
    void dequantize(int16_t block[8][8]);

private:
    uint16_t length;
    uint8_t percision;
    uint8_t id;
    uint16_t **quant_table;
};

class Scan_header{
public:
	Scan_header() {};
	Scan_header(uint8_t **data);
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
    jpeg_image(const char* path);
    jpeg_image(void *data, uint64_t size);
	Frame_header get_frame_header() {return frame_header;};
	Huffman_table get_huffman_table(int i, int j) {return huffman_tables[i][j];};
	Quantization_table get_quantization_table(int i) {return quantization_tables[i];};
	Restart_interval get_restart_interval() {return restart_interval;};
	struct Image_block get_image_block(uint32_t index);
    void decode_quantization_tables(uint8_t **data);
    void decode_huffman_tables(uint8_t **data);
	void decode_scan(uint8_t **data);
	void setup_image();
	void entropy_decode(Scan_header header, uint8_t **cpy_data);
	uint8_t decode_dc(int16_t data_block[8][8], uint8_t **cpy_data, Huffman_table huff, int16_t *pred, bool *EOS);
	uint8_t decode_ac(int16_t data_block[8][8], uint8_t **cpy_data, Huffman_table huff, bool *EOS);
private:
    void find_markers();
	uint8_t get_bit(uint8_t **cpy_data, bool *EOS);
	int16_t coefficient_decoding(uint16_t value, uint8_t size);
	void IDCT(int16_t data_block[8][8]);
	void convert_RGB();
    std::filesystem::path p;
    std::ifstream image;
	//Used for entropy decoding
	uint8_t pos = 0;
	uint8_t byte;
    uint8_t *data;
    uint64_t size = 0;
	bool frame_header_read;
    Frame_header frame_header;
	std::vector<Scan_header> scan_headers;
	uint8_t num_quantization_tables;
	Quantization_table quantization_tables[4];
	std::vector<Comment> comments;
	uint8_t num_huffman_tables;
	Huffman_table huffman_tables[2][4];
	std::vector<APP_header> app_headers;
	bool restart_interval_read;
	Restart_interval restart_interval;
	struct size image_block_size;
	struct Image_block **decoded_image_data;
	struct RGB *RGB_pixel_data;
};
