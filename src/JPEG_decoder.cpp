#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <cassert>
#include <vector>
#include <cstdio>
#include <cstdlib>

uint8_t cur_byte;
uint8_t cur_marker;
int8_t pos = 7;
std::vector<uint8_t> MCU = {};

uint8_t zigzag[64] = {0x00, 0x10, 0x01, 0x02, 0x11, 0x20, 0x30, 0x21,
					  0x12, 0x03, 0x04, 0x13, 0x22, 0x31, 0x40, 0x50,
					  0x41, 0x32, 0x23, 0x14, 0x05, 0x06, 0x16, 0x24,
					  0x33, 0x42, 0x51, 0x60, 0x70, 0x61, 0x52, 0x43,
					  0x32, 0x25, 0x16, 0x07, 0x17, 0x26, 0x35, 0x44,
					  0x53, 0x62, 0x71, 0x72, 0x63, 0x54, 0x45, 0x36,
					  0x27, 0x37, 0x46, 0x55, 0x64, 0x73, 0x74, 0x65,
					  0x56, 0x47, 0x57, 0x66, 0x75, 0x76, 0x67, 0x77};

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

JFIF_header *header;
QTable *qtable;
DCTheader *dctheader;
std::vector<HTable *> htables;
HTable *htable;
Scan_header *scanheader;

std::vector<Data_block *> MCU_block = {};

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

int main(){
    std::filesystem::path p = "..\\example\\cat.jpg";
    std::ifstream image = open_image(p);

    image.read(reinterpret_cast<char*>(&cur_byte), 1);
    //Find start of image
    if((cur_marker = find_marker(&image)) != 0xD8){
        std::cout << "Jpeg not detected\n";
        return 1;
    }

    //Need to add more possible header formats such as TIFF
    //Currently only FIFF is supported
    while((cur_marker = find_marker(&image)) != 0xD9){ //0xD9 indicating end of image
        switch(cur_marker){
            case 0xE0:
                std::cout << "Header found\n";
                header = read_header(&image);
                break;
            case 0xDB:
                //Read and Store Quantization Table
                
                std::cout << "Quantized table found\n";
                qtable = read_QTable(&image);
                break;
            case 0xC0:
                //Read and Store Baseline DCT

                std::cout << "BaseLine DCT found\n";
                dctheader = read_DCTheader(&image);
                break;
            case 0xC2:
                //Read and Store Progressive DCT

                std::cout << "Progressive DCT found\n";
                break;
            case 0xC4:
                //Read and Store one or more Huffman tables

                std::cout << "Huffman found\n";
                htable = read_HTable(&image);
                htables.push_back(htable);


                /*
                //For debugging purposes
                std::cout << "Length " << (int) htable->length << "\n";
                std::cout << "Type " << (int) htable->type << "\n";
                std::cout << "TableID " << (int) htable->table_ID << "\n";

                for(int i = 0; i < 16; i++){
                    std::cout << "bit length " << std::hex <<  i+1 << " Min Symbol Code: " <<  (int) htable->min_symbol[i] <<" Max Symbol Code: " <<  (int) htable->max_symbol[i] << "\n";

                    std::cout << "Symbols: \n";

                    for(int j = 0; j < htable->symbol_array[i].size(); j++){
                            std::cout << (int) htable->symbol_array[i][j] << " ";
                    }

                    std::cout << "\n";
                }
                */

                break;
            case 0xDD:
                //Defines Restart Interval

                std::cout << "Restart found\n";
                break;
            case 0xDA:
                //Top to Bottom Scan of the image
                /* Begins a top-to-bottom scan of the image.
                 In baseline DCT JPEG images, there is generally
                 a single scan. Progressive DCT JPEG images
                 usually contain multiple scans. This marker
                 specifies which slice of data it will contain,
                 and is immediately followed by entropy-coded data.
                */

                std::cout << "Scan of the image found\n";
                scanheader = read_Scan_header(&image);

                image.read(reinterpret_cast<char*>(&cur_byte), 1);
                calculate_MCU();
				create_MCU_block();
				read_MCU(&image);

                break;
            /*
            case 0xEn: //For Application specific header that are not 0xE0

                break;
            */
            case 0xFE:
                //contains a comment

                std::cout << "Comment found\n";
                read_comment(&image);

                break;
        }
    }
}

std::ifstream open_image(std::filesystem::path p){
    assert(exists(p));
    return std::ifstream (p, std::ios::binary);
}

uint8_t find_marker(std::ifstream *image){
    while(image->peek() != EOF){
        if(cur_byte != 0xff){
            image->read(reinterpret_cast<char*>(&cur_byte), 1);
        }
        else{
            image->read(reinterpret_cast<char*>(&cur_byte), 1);
            if(cur_byte != 0x00){
                cur_marker = cur_byte;
                image->read(reinterpret_cast<char*>(&cur_byte), 1);
                return cur_marker;
            }
            image->read(reinterpret_cast<char*>(&cur_byte), 1);
        }
    }
    return cur_marker;
}

JFIF_header *read_header(std::ifstream *image){
    uint16_t Length = cur_byte;
    image->read(reinterpret_cast<char*>(&cur_byte), 1);
    Length = (Length << 8) + cur_byte;

    uint8_t *header = new uint8_t[Length - 2];
    image->read(reinterpret_cast<char*>(header), Length - 2);

    uint64_t Identfier = 0;
    for(int i = 0; i < 5; i++)
        Identfier = (Identfier << 8) + *((uint8_t *) header + i);

    uint16_t JFIF_Version = (*((uint8_t *) header + 6) << 8) + *((uint8_t *) header + 5);
    uint16_t Xdensity = (*((uint8_t *) header + 8) << 8) + *((uint8_t *) header + 9);
    uint16_t Ydensity = (*((uint8_t *) header + 10) << 8) + *((uint8_t *) header + 11);
    uint8_t Density_Units = *((uint8_t *) header + 7);
    uint8_t Xthumbnail = *((uint8_t *) header + 12);
    uint8_t Ythumbnail = *((uint8_t *) header + 13);

    free(header);

    return new JFIF_header(Length, Identfier, JFIF_Version, Density_Units, Xdensity, Ydensity, Xthumbnail, Ythumbnail);
}

QTable *read_QTable(std::ifstream *image){
	uint16_t Length = cur_byte;
    image->read(reinterpret_cast<char*>(&cur_byte), 1);
    Length = (Length << 8) + cur_byte;

    image->read(reinterpret_cast<char*>(&cur_byte), 1);
    uint8_t Size = ((cur_byte >> 4) & 0xF) + 1;
    uint8_t ID = cur_byte & 0xF;

    uint16_t *quant_table = new uint16_t[64];
    if(Size == 2){
        for(int i = 0; i < 64; i++){
            image->read(reinterpret_cast<char*>(&cur_byte), 1);
            quant_table[i] = cur_byte;
            image->read(reinterpret_cast<char*>(&cur_byte), 1);
            quant_table[i] = (quant_table[i] << 8) + cur_byte;
        }
    }
    else if(Size == 1)
        for(int i = 0; i < 64; i++){
            image->read(reinterpret_cast<char*>(&cur_byte), 1);
            quant_table[i] = cur_byte;
        }
    return new QTable(Length, Size, ID, quant_table);
}

DCTheader *read_DCTheader(std::ifstream *image){
    uint16_t Length = cur_byte;
    image->read(reinterpret_cast<char*>(&cur_byte), 1);
    Length = (Length << 8) + cur_byte;

    image->read(reinterpret_cast<char*>(&cur_byte), 1);
    uint8_t Sample_percision = cur_byte;

    image->read(reinterpret_cast<char*>(&cur_byte), 1);
    uint16_t Height = cur_byte;
    image->read(reinterpret_cast<char*>(&cur_byte), 1);
    Height = (Height << 8) + cur_byte;

    image->read(reinterpret_cast<char*>(&cur_byte), 1);
    uint16_t Width = cur_byte;
    image->read(reinterpret_cast<char*>(&cur_byte), 1);
    Width = (Width << 8) + cur_byte;

    image->read(reinterpret_cast<char*>(&cur_byte), 1);
    uint8_t Num_chans = cur_byte;

    Channel_info *Channel_infos = new Channel_info[Num_chans];
    for(uint8_t i = 0; i < Num_chans; i++){
        image->read(reinterpret_cast<char*>(&cur_byte), 1);
        Channel_infos[i].identifier = cur_byte;
        image->read(reinterpret_cast<char*>(&cur_byte), 1);
        Channel_infos[i].horz_sampling = (cur_byte >> 4) & 0xF;
        Channel_infos[i].vert_sampling = (cur_byte & 0xF);
        image->read(reinterpret_cast<char*>(&cur_byte), 1);
        Channel_infos[i].qtableID = cur_byte;
    }

    return new DCTheader(Length, Sample_percision, Height, Width, Num_chans, Channel_infos);
}

void calculate_MCU(){
	uint8_t Num_chans = scanheader->num_chans;
	if(Num_chans == 1)
		MCU.push_back(1);
	for(int i = 0; i < Num_chans; i++){
		uint8_t ComponentID = scanheader->chan_specs[i].componentID;
		bool found = false;
		for(Channel_info *j = dctheader->chan_infos; j < (dctheader->chan_infos + dctheader->num_chans); j++){ 
			if(j->identifier == ComponentID){
				MCU.push_back(j->horz_sampling * j->vert_sampling);
				found = true;
				break;
			}	
		}
		if(!found)
			std::cout << "ID of component not found\n";
	}
}

void create_MCU_block(){
	for(int i = 0; i < MCU.size(); i++){
		for(int j = 0; j < MCU[i]; j++){
			uint8_t ComponentID = scanheader->chan_specs[i].componentID;
			Channel_info *k = dctheader->chan_infos;
			for(;k < (dctheader->chan_infos + dctheader->num_chans); k++){ 
				if(k->identifier == ComponentID)
					break;
			}
			MCU_block.push_back(new Data_block(j%(k->vert_sampling), j/(k->horz_sampling)));
		}
	}
}

void read_MCU(std::ifstream *image){
	int block_pos = 0;
	for(int i = 0; i < MCU.size(); i++){
		for(int j = 0; j < MCU[i]; j++)
			read_data_block(MCU_block[block_pos++], image, i);
	}
}

void read_data_block(Data_block *data_block, std::ifstream *image, uint8_t id){
	//search for Huffman_DC
	uint8_t Huff_DC_ID = scanheader->chan_specs[id].Huffman_DC;
	uint8_t Huff_AC_ID = scanheader->chan_specs[id].Huffman_AC;
	
	uint8_t i = 0;
	for(; i < htables.size(); i++){
		if(htables[i]->type == 0 && htables[i]->table_ID == Huff_DC_ID)
			break;
	}

	uint8_t j = 0;
	for(; j < htables.size(); j++){
		if(htables[j]->type == 1 && htables[j]->table_ID == Huff_AC_ID)
			break;
	}

	data_block->data[0][0] = decode_DC_coefficient(image, htables[i]);
	uint8_t coeff_count = 1;
	while(coeff_count < 64){
		AC_coefficient *AC = decode_AC_coefficient(image, htables[j]);//find the correct htable
		if(AC->EOB){
            for(int i = 0; i < 7; i++){
                for(int j = 0; j < 7; j++)
                    std::cout << data_block->data[i][j] << " ";
                std::cout << std::endl;
            }
			return;
        }
		else{
			coeff_count += AC->run_length;
			uint8_t decoded_horz = (zigzag[coeff_count] >> 4) & 0xF;
			uint8_t decoded_vert = (zigzag[coeff_count]) & 0xF;
			data_block->data[decoded_vert][decoded_horz] = AC->value;
		}
	}

    for(int i = 0; i < 7; i++){
		for(int j = 0; j < 7; j++)
			std::cout << data_block->data[i][j];
        std::cout << std::endl;
	}

}

Scan_header *read_Scan_header(std::ifstream *image){
    uint16_t Length = cur_byte;
    image->read(reinterpret_cast<char*>(&cur_byte), 1);
    Length = (Length << 8) + cur_byte;

    uint8_t *Header = new uint8_t[Length - 2];

    image->read(reinterpret_cast<char*>(Header), Length - 2);

	uint8_t i = 0;
    uint8_t Num_chans = Header[i++];
    Chan_specifier *Chan_specs = new Chan_specifier[Num_chans];
    for(uint8_t j = 0; j < Num_chans; j++){
    	Chan_specs[j].componentID = Header[i++];
		Chan_specs[j].Huffman_DC = (Header[i] >> 4) & 0xF;
		Chan_specs[j].Huffman_AC = Header[i++] & 0xF;
    }
	uint8_t Spectral_start = Header[i++];
	uint8_t Spectral_end = Header[i++];
	uint8_t Successive_approx = Header[i++];

    free(Header);
    return new Scan_header(Length, Num_chans, Chan_specs, Spectral_start, Spectral_end, Successive_approx);
}

HTable *read_HTable(std::ifstream *image){
    uint16_t Length = cur_byte;
	image->read(reinterpret_cast<char*>(&cur_byte), 1);
	Length = (Length << 8) + cur_byte;

    image->read(reinterpret_cast<char*>(&cur_byte), 1);
    uint8_t Type = (cur_byte >> 4) & 0xF;
    uint8_t Table_ID = cur_byte & 0xF;

    uint8_t Hcodes_lengths[16];
    image->read(reinterpret_cast<char*>(Hcodes_lengths), 16);

    uint8_t *Coded_symbol_array = new uint8_t [Length - 19];
    image->read(reinterpret_cast<char*>(Coded_symbol_array), (std::streamsize) Length - 19);

    std::vector<uint8_t> *Symbol_array = new std::vector<uint8_t>[16];
    interpret_HTable(Hcodes_lengths, Coded_symbol_array, Symbol_array);

    int16_t *Min_symbol = new int16_t[16];
    int16_t *Max_symbol = new int16_t[16];
    create_max_min_symbols(Min_symbol, Max_symbol, Symbol_array);

    free(Coded_symbol_array);
    return new HTable(Length, Type, Table_ID, Min_symbol, Max_symbol, Symbol_array);
}

void interpret_HTable(uint8_t Hcodes_lengths[16], uint8_t *Coded_symbol_array, std::vector<uint8_t> *Symbol_array){
    uint8_t Coded_symbol_count = 0;
    for(int i = 0; i < 16; i++){
        for(int j = 0; j < Hcodes_lengths[i]; j++){
            Symbol_array[i].push_back(Coded_symbol_array[Coded_symbol_count++]);
        }
    }
}

void create_max_min_symbols(int16_t min_symbol[16],int16_t max_symbol[16], std::vector<uint8_t> *Symbol_array){
    int last_non_zero_length = -1;
    while(Symbol_array[++last_non_zero_length].size() == 0){
        min_symbol[last_non_zero_length] = -1;
        max_symbol[last_non_zero_length] = -1;
    }

    min_symbol[last_non_zero_length] = 0;
    max_symbol[last_non_zero_length] = Symbol_array[last_non_zero_length].size() - 1;

    for(int i = last_non_zero_length+1; i < 16; i++){
        if(Symbol_array[i].size() > 0){
            min_symbol[i] = (max_symbol[last_non_zero_length] + 1) << 1;
            max_symbol[i] = min_symbol[i] + Symbol_array[i].size() - 1;
            last_non_zero_length = i;
        }
        else{
            min_symbol[i] = -1;
            max_symbol[i] = -1;
        }
    }
}

void read_comment(std::ifstream *image){
    uint16_t Length = cur_byte;
    image->read(reinterpret_cast<char*>(&cur_byte), 1);
    Length = (Length << 8) + cur_byte;

    char *comment = new char[Length - 1];
    image->read(comment, Length - 2);
    comment[Length - 2] = '\0';

    std::cout << comment << "\n";

    free(comment);
}

int decode_DC_coefficient(std::ifstream *image, HTable *htable){
    uint16_t Size = 0;
    uint8_t Size_read = 0;
    int16_t Value = 0;

    while(!image->eof() && Size_read < 12){
        if(pos < 0){
            if(cur_byte == 0xFF)
                image->read(reinterpret_cast<char*>(&cur_byte), 1);
            image->read(reinterpret_cast<char*>(&cur_byte), 1);
            pos = 7;
        }
        Size = (Size << 1) + ((cur_byte >> pos--) & 1);
        Size_read++;

        if(Size >= htable->min_symbol[Size_read - 1] && Size <= htable->max_symbol[Size_read - 1]){
            bool Found = false;
            for(uint8_t i = 0; i < htable->symbol_array[Size_read - 1].size(); i++){
                if(Size == htable->min_symbol[Size_read - 1] + i){
                    Size = htable->symbol_array[Size_read - 1][i];
                    Found = true;
                    break;
                }
            }
            if(Found) break;
        }
    }

    uint8_t i = 0;
    while(!image->eof() && i++ < Size){
        if(pos < 0){
            image->read(reinterpret_cast<char*>(&cur_byte), 1);
            pos = 7;
        }
        Value = (Value << 1) + ((cur_byte >> pos--) & 1);
    }

    //For signed numbers
    if(Value < (1 << (Size - 1))){
        int difference = 2 * ((1 << (Size - 1)) - Value) - 1;
        Value = - (Value + difference);
    }
    return Value;
}

AC_coefficient *decode_AC_coefficient(std::ifstream *image, HTable *htable){
    uint16_t RLength_or_Size = 0;
    uint8_t RLength_or_Size_read = 0;
    int16_t Value = 0;

    while(!image->eof() && RLength_or_Size_read < 12){
        if(pos < 0){
            if(cur_byte == 0xFF)
                image->read(reinterpret_cast<char*>(&cur_byte), 1);
            image->read(reinterpret_cast<char*>(&cur_byte), 1);
            pos = 7;
        }
        RLength_or_Size = (RLength_or_Size << 1) + ((cur_byte >> pos--) & 1);
        RLength_or_Size_read++;
        if(RLength_or_Size >= htable->min_symbol[RLength_or_Size_read - 1] && RLength_or_Size <= htable->max_symbol[RLength_or_Size_read - 1]){
            bool Found = false;
            for(uint8_t i = 0; i < htable->symbol_array[RLength_or_Size_read - 1].size(); i++){
                if(RLength_or_Size == htable->min_symbol[RLength_or_Size_read - 1] + i){
                    RLength_or_Size = htable->symbol_array[RLength_or_Size_read - 1][i];
                    Found = true;
                    break;
                }
            }
            if(Found) break;
        }
    }
    if(RLength_or_Size == 0)
        return new AC_coefficient(true, 0, 0);

    uint8_t Size = RLength_or_Size & 0xF;
    uint8_t RLength = (RLength_or_Size >> 4) & 0xF;

    uint8_t i = 0;
    while(!image->eof() && i++ < Size){
        if(pos < 0){
            image->read(reinterpret_cast<char*>(&cur_byte), 1);
            pos = 7;
        }
        Value = (Value << 1) + ((cur_byte >> pos--) & 1);
    }

    //For signed numbers
    if(Value < (1 << (Size - 1))){
        int difference = 2 * ((1 << (Size - 1)) - Value) - 1;
        Value = - (Value + difference);
    }
    return new AC_coefficient(false, RLength, Value);
}


