#include "JPEG_decoder.hh"

uint8_t test_frame_header_data[] = {0xFF, 0xC0, 0x00, 0x11, 0x08, 0x00, 0x08, 0x00, 0x08, 0x03, 0x01, 0x22, 0x00, 0x02, 0x11, 0x01, 0x03, 0x11, 0x01, '\0'};
uint8_t test_scan_header_data[] = {0xFF, 0xDA, 0x00, 0x0C, 0x03, 0x01, 0x00, 0x02, 0x11, 0x03, 0x11, 0x00, 0x3F, 0x00, '\0'};
uint8_t test_quantization_table_data[] = {0xFF, 0xDB, 0x00, 0x43, 0x00, 0x03, 0x02, 0x02, 0x03, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x04, 0x03, 0x03, 0x04, 0x05, 0x08, 0x05, 0x05, 0x04, 0x04, 0x05, 0x0A, 0x07, 0x07, 0x06, 0x08, 0x0C, 0x0A, 0x0C, 0x0C, 0x0B, 0x0A, 0x0B, 0x0B, 0x0D, 0x0E, 0x12, 0x10, 0x0D, 0x0E, 0x11, 0x0E, 0x0B, 0x0B, 0x10, 0x16, 0x10, 0x11, 0x13, 0x14, 0x15, 0x15, 0x15, 0x0C, 0x0F, 0x17, 0x18, 0x16, 0x14, 0x18, 0x12, 0x14, 0x15, 0x14, '\0'};
uint8_t test_JFIF_header_data[] = {0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01, 0x01, 0x01, 0x00, 0x60, 0x00, 0x60, 0x00, 0x00};
uint8_t test_huffman_table_data[] = {0xFF, 0xC4, 0x00, 0x24, 0x11, 0x00, 0x03, 0x00, 0x02, 0x02, 0x03, 0x01, 0x00, 0x02, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x11, 0x12, 0x31, 0x13, 0x21, 0x41, 0x51, 0x42, 0x71, 0x04, 0x22, 0x61, 0x32, '\0'};
int main(){
    jpeg_image jpeg = jpeg_image("..//example//jpeg422jfif.jpg");

	/*
	uint8_t *test_DRI_marker_data = new uint8_t[]{0x00, 0x04, 0x05, 0x04};
	uint8_t *cpy_test_DRI_marker_data = test_DRI_marker_data;

	Restart_interval test_restart_interval = Restart_interval(&cpy_test_DRI_marker_data);
	std::cout << "Interval: " << (int) test_restart_interval.get_num_MCUs() << std::endl;
	*/

    /*
    Frame_header test_frame_header = jpeg.get_frame_header(); 
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

    /*
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
    */
	/*
	for(int i = 0; i < 2; i++){
			Quantization_table test_quantization_table = jpeg.get_quantization_table(i);
			std::cout << "Length: " << (int) test_quantization_table.get_length() << std::endl;
			std::cout << "Percision: " << (int) test_quantization_table.get_precision() << std::endl;
			std::cout << "Id: " << (int) test_quantization_table.get_id() << std::endl;

			uint16_t **quant_table = test_quantization_table.get_quantization_table();
			for(int j = 0; j < 64; j++)
				std::cout << (int) quant_table[j/8][j%8] << " ";
			std::cout << std::endl;
    }
	*/
	/*
	for(int i = 0; i < 2; i++){
		for(int j = 0; j < 2; j++){
			Huffman_table huffman_table = jpeg.get_huffman_table(i, j);
	
			std::cout << std::dec << "length: " << (int) huffman_table.get_length() << std::endl;
			std::cout << "type: " << (int) huffman_table.get_type() << std::endl;	
			std::cout << "table_id: " << (int) huffman_table.get_table_id() << std::endl;
			for(int i = 0; i < 16; i++){
				std::cout << "Num of codes for " << i+1 << ": "  << std::hex << (int) huffman_table.get_num_codes_len_i()[i] << "\t Min code value: " << (int) huffman_table.get_min_code_values()[i] << "\t Max code value: " << (int) huffman_table.get_max_code_values()[i] << std::endl;
				int symbol_size = huffman_table.get_num_codes_len_i()[i];
				for(int j = 0; j < symbol_size; j++){
					std::cout << (int) huffman_table.get_symbol_arrays()[i][j] << " ";
				}	
				std::cout << std::endl;
			}
		}
	}
	*/
}
