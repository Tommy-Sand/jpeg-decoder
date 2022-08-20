#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <cassert>
#include <vector>

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


std::ifstream open_image(std::filesystem::path p);
uint8_t find_marker(std::ifstream *image);
JFIF_header *read_header(std::ifstream *image);

int main(){
    std::filesystem::path p = "..\\example\\cat.jpg";
    std::ifstream image {p};

    cur_byte = image.get();

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
                JFIF_header *header = read_header(&image);
                break;
            case 0xDB:
                //Read and Store Quantization Table
                break;
            case 0xC0:
                //Read and Store Baseline DCT
                break;
            case 0xC2:
                //Read and Store Progressive DCT
                break;
            case 0xC4:
                //Read and Store one or more Huffman tables
                break;
            case 0xDD:
                //Defines Restart Interval
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
                break;
            case 0xDn: //n from 0 to 7
                //Restart
                /*
                Inserted every r macroblocks, where r is the restart
                interval set by a DRI marker. Not used if there was 
                no DRI marker. The low three bits of the marker code
                cycle in value from 0 to 7.
                */
                break;
            case 0xEn: //For Application specific header that are not 0xE0

                break;
            case 0xFE:
                //contains a comment
                break;
        }
    }
}

std::ifstream open_image(std::filesystem::path p){
    assert(exists(p));
    return std::ifstream {p};
}

uint8_t find_marker(std::ifstream *image){
    while(!image->eof()){
        if(cur_byte != 0xff){
            cur_byte = image->get();
        }
        else{
            cur_byte = image->get();
            if(cur_byte != 0x00){
                marker = cur_byte;
                cur_byte = image->get();
                break;
            }
            cur_byte = image->get();
        }
    }
    return marker;
}

JFIF_header *read_header(std::ifstream *image){
    uint16_t length = cur_byte;
    cur_byte = image->get();
    length = (length << 8) + cur_byte;
    
    char *header = new char[length - 1];
    image->get(header, (std::streamsize) length);

    uint64_t Identfier = 0;
    for(int i = 0; i < 5; i++)
        Identfier = (Identfier << 8) + *((uint8_t *) header + i);

    uint16_t JFIF_Version = (*((uint8_t *) header + 6) << 8) + *((uint8_t *) header + 5);
    uint16_t Xdensity = (*((uint8_t *) header + 8) << 8) + *((uint8_t *) header + 9);
    uint16_t Ydensity = (*((uint8_t *) header + 10) << 8) + *((uint8_t *) header + 11);
    uint8_t Density_Units = *((uint8_t *) header + 7);
    uint8_t Xthumbnail = *((uint8_t *) header + 12);
    uint8_t Ythumbnail = *((uint8_t *) header + 13);

    return new JFIF_header(length, Identfier, JFIF_Version, Density_Units, Xdensity, Ydensity, Xthumbnail, Ythumbnail);
}

QTable *read_QTable(std::ifstream *image){

}

DCTable *read_DCTable(std::ifstream *image, bool ProgressiveDCTable){

}

HTable *read_HTable(std::ifstream *image){

}

void *read_comment(std::ifstream *image){
    
}
