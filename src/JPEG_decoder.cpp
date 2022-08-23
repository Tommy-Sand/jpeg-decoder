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

class DCTable{

};

class HTable{
public:
    HTable(uint16_t Length, uint8_t Type, uint8_t Table_ID, uint8_t H_codes[16], uint8_t *Symbol_array):
    length{Length}, type{Type}, table_ID{Table_ID}, symbol_array{Symbol_array} {
        std::memcpy(h_codes, H_codes, 16);
    }

    uint16_t length;
    uint8_t type;
    uint8_t table_ID;
    uint8_t h_codes[16];
    uint8_t *symbol_array;
};

JFIF_header *header;
QTable *qtable;
DCTheader *dctheader;
HTable *htable;

std::ifstream open_image(std::filesystem::path p);
uint8_t find_marker(std::ifstream *image);
JFIF_header *read_header(std::ifstream *image);
QTable *read_QTable(std::ifstream *image);
DCTable *read_DCTable(std::ifstream *image, bool ProgressiveDCTable);
DCTheader *read_DCTheader(std::ifstream *image);
HTable *read_HTable(std::ifstream *image);
void *read_comment(std::ifstream *image);

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
                std::cout << "BaseLine DCT found\n";
                //Read and Store Baseline DCT
                dctheader = read_DCTheader(&image);
                break;
            case 0xC2:
                std::cout << "Progressive DCT found\n";
                //Read and Store Progressive DCT
                break;
            case 0xC4:
                std::cout << "Huffman found\n";
                htable = read_HTable(&image);
                //Read and Store one or more Huffman tables
                break;
            case 0xDD:
                std::cout << "Restart found\n";
                //Defines Restart Interval
                break;
            case 0xDA:
                std::cout << "Scan of the image found\n";
                //Top to Bottom Scan of the image
                /* Begins a top-to-bottom scan of the image.
                 In baseline DCT JPEG images, there is generally
                 a single scan. Progressive DCT JPEG images
                 usually contain multiple scans. This marker
                 specifies which slice of data it will contain,
                 and is immediately followed by entropy-coded data. 
                */
                
                break;
            /*
            case 0xDn: //n from 0 to 7
                //Restart
                
                Inserted every r macroblocks, where r is the restart
                interval set by a DRI marker. Not used if there was 
                no DRI marker. The low three bits of the marker code
                cycle in value from 0 to 7.
                
                break;
            
            case 0xEn: //For Application specific header that are not 0xE0

                break;
            */
            case 0xFE:
                std::cout << "Comment found\n";
                read_comment(&image);
                //contains a comment
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

DCTable *read_DCTable(std::ifstream *image, bool ProgressiveDCTable){
    
}

HTable *read_HTable(std::ifstream *image){
    uint16_t Length = cur_byte;
	image->read(reinterpret_cast<char*>(&cur_byte), 1);
	Length = (Length << 8) + cur_byte;

    image->read(reinterpret_cast<char*>(&cur_byte), 1);
    uint8_t Type = (cur_byte >> 4) & 0xF;
    uint8_t Table_ID = cur_byte & 0xF;

    uint8_t H_codes[16];
    image->read(reinterpret_cast<char*>(H_codes), 16);

    uint8_t *Symbol_array = new uint8_t [Length - 19];
    image->read(reinterpret_cast<char*>(Symbol_array), (std::streamsize) Length - 19);

    return new HTable(Length, Type, Table_ID, H_codes, Symbol_array);
}

void *read_comment(std::ifstream *image){
    uint16_t Length = cur_byte;
    image->read(reinterpret_cast<char*>(&cur_byte), 1);
    Length = (Length << 8) + cur_byte;

    char *comment = new char[Length - 1];
    image->read(comment, Length - 2);
    comment[Length - 2] = '\0';

    std::cout << comment << "\n";

    free(comment);
    return 0;
}
