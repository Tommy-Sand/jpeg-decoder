#include "jpeg_decoder.hh"

uint8_t test_frame_header_data[] = {0xFF, 0xC0, 0x00, 0x11, 0x08, 0x00, 0x08, 0x00, 0x08, 0x03, 0x01, 0x22, 0x00, 0x02, 0x11, 0x01, 0x03, 0x11, 0x01, '\0'};

int main(){
    jpeg_image jpeg = jpeg_image("..//example//u.jpg");

    Frame_header test_frame_header = Frame_header(test_frame_header_data);
    std::cout << "Length: " << (int) test_frame_header.get_length() << std::endl;
    std::cout << "Precision: "<< (int) test_frame_header.get_precision() << std::endl;
    std::cout << "Height: " << (int) test_frame_header.get_height() << std::endl;
    std::cout << "Width: " << (int) test_frame_header.get_width() << std::endl;
    std::cout << "Num_Chans: " << (int) test_frame_header.get_num_chans() << std::endl;
    for(int i = 0; i < test_frame_header.get_num_chans(); i++){
        std::cout << "Channel info " << i << std::endl;
        struct Channel_info *chan_info = test_frame_header.get_chan_info(i);
        std::cout << "id: " << (int) chan_info->id << std::endl;
        std::cout << "horz sampling: " << (int) chan_info->horz_sampling << std::endl;
        std::cout << "vert sampling: " <<(int) chan_info->vert_sampling << std::endl;
        std::cout << "qtableID: "<<(int) chan_info->qtableID << std::endl;
    }
}