#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <cassert>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cmath>

class JFIF_header{
public:
    JFIF_header(uint16_t Length, uint64_t Identfier, uint16_t JFIF_Version, uint8_t Dens_units, uint16_t Xdens, uint16_t Ydens, uint8_t Xthumbnail, uint8_t Ythumbnail)
    : length{Length}, identfier{Identfier}, JFIF_version{JFIF_Version}, dens_units{Dens_units}, xdens{Xdens}, ydens{Ydens}, xthumbnail{Xthumbnail}, ythumbnail{Ythumbnail},
    thumb {std::vector<std::vector<std::vector<uint8_t>>> (3, std::vector<std::vector<uint8_t>> (Ythumbnail, std::vector<uint8_t> (Xthumbnail)))} {}

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

class Channel_info{
public:
    uint8_t identifier;
    uint8_t horz_sampling;
    uint8_t vert_sampling;
    uint8_t qtableID;
};

class DCTheader{
public:
    DCTheader(uint16_t Length, uint8_t Sample_percision, uint16_t Height, uint16_t Width, uint8_t Num_chans, Channel_info *Chan_infos)
    : length{Length}, sample_percision{Sample_percision}, height{Height}, width{Width}, num_chans{Num_chans}, chan_infos{Chan_infos} {}
    uint16_t length;
    uint8_t sample_percision;
    uint16_t height;
    uint16_t width;
    uint8_t num_chans;
    Channel_info *chan_infos;
};

class QTable{
public:
    QTable(uint16_t Length, uint8_t Size, uint8_t ID, uint16_t *Quant_table)
    : length{Length}, size{Size}, id{ID}, quant_table{Quant_table} {}

    uint16_t length;
    uint8_t size;
    uint8_t id;
    uint16_t *quant_table;
};

class Chan_specifier{
public:
    uint8_t componentID;
    uint8_t Huffman_DC;
    uint8_t Huffman_AC;
};

class Scan_header{
public:
	Scan_header(uint16_t Length, uint8_t Num_chans, Chan_specifier *Chan_specs, uint8_t Spectral_start, uint8_t Spectral_end, uint8_t Successive_approx): length{Length}, num_chans{Num_chans}, chan_specs{Chan_specs}, spectral_start{Spectral_start}, spectral_end{Spectral_end}, successive_approx{Successive_approx} {}

    uint16_t length;
    uint8_t num_chans;
    Chan_specifier *chan_specs;
    uint8_t spectral_start;
    uint8_t spectral_end;
    uint8_t successive_approx;
};

class HTable{
public:
    HTable(uint16_t Length, uint8_t Type, uint8_t Table_ID, int16_t *Min_symbol, int16_t *Max_symbol, std::vector<uint8_t> *Symbol_array):
    length{Length}, type{Type}, table_ID{Table_ID}, min_symbol{Min_symbol}, max_symbol{Max_symbol}, symbol_array{Symbol_array} {}

    uint16_t length;
    uint8_t type;
    uint8_t table_ID;
    int16_t *min_symbol;
    int16_t *max_symbol;
    std::vector<uint8_t> *symbol_array;
};

class AC_coefficient{
public:
    AC_coefficient(bool EOB, uint8_t Run_length, int Value)
    :EOB{EOB}, run_length{Run_length}, value{Value} {}

    bool EOB;
    uint8_t run_length;
    int value;
};

class Data_block{
public:
	Data_block(uint8_t Horz, uint8_t Vert): horz{Horz}, vert{Vert} {}
	
	uint8_t horz;
	uint8_t vert;
	int16_t data[8][8] = {0};
};

uint8_t cur_byte;
uint8_t cur_marker;
int8_t pos = 7;
std::vector<uint8_t> MCU = {};
JFIF_header *header;
QTable *qtable;
DCTheader *dctheader;
std::vector<HTable *> htables;
HTable *htable;
Scan_header *scanheader;
std::vector<Data_block *> MCU_block = {};
uint8_t zigzag[64] = {0x00, 0x10, 0x01, 0x02, 0x11, 0x20, 0x30, 0x21,
					  0x12, 0x03, 0x04, 0x13, 0x22, 0x31, 0x40, 0x50,
					  0x41, 0x32, 0x23, 0x14, 0x05, 0x06, 0x16, 0x24,
					  0x33, 0x42, 0x51, 0x60, 0x70, 0x61, 0x52, 0x43,
					  0x32, 0x25, 0x16, 0x07, 0x17, 0x26, 0x35, 0x44,
					  0x53, 0x62, 0x71, 0x72, 0x63, 0x54, 0x45, 0x36,
					  0x27, 0x37, 0x46, 0x55, 0x64, 0x73, 0x74, 0x65,
					  0x56, 0x47, 0x57, 0x66, 0x75, 0x76, 0x67, 0x77};

std::ifstream open_image(std::filesystem::path p);
uint8_t find_marker(std::ifstream *image);
JFIF_header *read_header(std::ifstream *image);
QTable *read_QTable(std::ifstream *image);
DCTheader *read_DCTheader(std::ifstream *image);
void calculate_MCU();
void create_MCU_block();
void read_MCU(std::ifstream *image);
void read_data_block(Data_block *data_block, std::ifstream *image, uint8_t id);
HTable *read_HTable(std::ifstream *image);
void interpret_HTable(uint8_t Hcodes_lengths[], uint8_t*Coded_symbol_array, std::vector<uint8_t> *Symbol_array);
void create_max_min_symbols(int16_t min_symbol[16],int16_t max_symbol[16], std::vector<uint8_t> *Symbol_array);
void read_comment(std::ifstream *image);
Scan_header *read_Scan_header(std::ifstream *image);
int decode_DC_coefficient(std::ifstream *image, HTable *htable);
AC_coefficient *decode_AC_coefficient(std::ifstream *image, HTable *htable);