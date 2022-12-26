#include "jpeg_decoder.hh"

uint8_t test_frame_header_data[] = {0xFF, 0xC0, 0x00, 0x11, 0x08, 0x00, 0x08, 0x00, 0x08, 0x03, 0x01, 0x22, 0x00, 0x02, 0x11, 0x01, 0x03, 0x11, 0x01, '\0'};
uint8_t test_scan_header_data[] = {0xFF, 0xDA, 0x00, 0x0C, 0x03, 0x01, 0x00, 0x02, 0x11, 0x03, 0x11, 0x00, 0x3F, 0x00, '\0'};

int main(){
    jpeg_image jpeg = jpeg_image("..//example//u.jpg");
    /*
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
        std::cout << "qtableID: "<< (int) chan_info->qtableID << std::endl;
    }
    */
    Scan_header test_scan_header = Scan_header(test_scan_header_data);
    std::cout << "Length: " << (int) test_scan_header.get_length() << std::endl;
    std::cout << "Num Channels: " << (int) test_scan_header.get_num_chans() << std::endl;
    for(int i = 0; i < test_scan_header.get_num_chans(); i++){
        struct Chan_specifier *chan_specifier = test_scan_header.get_chan_spec(i);
        std::cout << "Channel specifier index: " << (int) i << std::endl;
        std::cout << "Channel specifier id: "<< (int) chan_specifier->componentID << std::endl;
        std::cout << "chan spec huffman DC index: " << (int) chan_specifier->Huffman_DC << std::endl;
        std::cout << "cahn spec huffman AC index: " <<(int) chan_specifier->Huffman_AC << std::endl;
    }
    std::cout << "spectral start: " << (int) test_scan_header.get_spectral_start() << std::endl;
    std::cout << "spectral end: " << (int) test_scan_header.get_spectral_end() << std::endl;
    std::cout << "prev approx: " << (int) test_scan_header.get_prev_approx() << std::endl;
    std::cout << "succ_approx: " << (int) test_scan_header.get_succ_approx() << std::endl;
}