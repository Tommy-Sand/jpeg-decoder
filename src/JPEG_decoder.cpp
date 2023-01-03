#include "JPEG_decoder.hh"


jpeg_image::jpeg_image(const char* path): p{path} {
	this->frame_header_read = false;
	this->num_quantization_tables = 0;
	this->num_huffman_tables = 0;
	this->restart_interval_read = 0;

    assert(std::filesystem::exists(p));
    image = std::ifstream (p, std::ios::binary);
    assert(image.is_open());
    assert(image.good());

    size = std::filesystem::file_size(p);
    data = new uint8_t[size+1];

    image.read(reinterpret_cast<char*>(data), size);
    data[size] = '\0';
    assert(!image.fail());

    image.close();
    assert(!image.bad());

    //find markers in data
    this->find_markers();
}

jpeg_image::jpeg_image(void *data, uint64_t size): data{(uint8_t *) data}, size{size} {
    this->find_markers();
}

void jpeg_image::find_markers(){
	uint8_t *cpy_data = this->data;
    //Look for marker 0xFFD8, indicating start of the image
   	bool SOI_marker = false;
	bool EOI_marker = false;
    while(!SOI_marker && cpy_data - data < size){
		uint8_t first_byte = *(cpy_data++);
		uint8_t second_byte = *(cpy_data++);
        if(first_byte == 0xFF && second_byte == 0xD8)
            SOI_marker = true;
    }
	if(!SOI_marker)
		return;

    //Look for all other markers
    while(!EOI_marker){
        if(*cpy_data == 0xFF && cpy_data - data < size && *(cpy_data+1) != 0xFF && *(cpy_data+1) != 0x00){
            switch(*(++cpy_data)){
                case 0xC0: //SOF Sequential
                case 0xC1: //SOF Sequential extended
                case 0xC2: //SOF progressive
                    this->frame_header = Frame_header(&cpy_data);
					this->setup_image();
					break;
                case 0xC4:
                    this->decode_huffman_tables(&(++cpy_data));
					break;
                //Restart markers from 0 to 7
                case 0xD0:
                case 0xD1:
                case 0xD2:
                case 0xD3:
                case 0xD4:
                case 0xD5:
                case 0xD6:
                case 0xD7:
                    //restart marker
					break;
                case 0xD9:
                    //EOI
					EOI_marker = true;
					break;
                case 0xDA:
                    //SOS
					this->scan_headers.push_back(Scan_header(&(++cpy_data)));
					this->entropy_decode(&(++cpy_data), this->scan_headers[this->scan_headers.size() - 1]);
					break;
                case 0xDB:
                    //Quantization
					this->decode_quantization_tables(&(++cpy_data));
                    break;
                case 0xDD:
                    //restart interval
					this->restart_interval = Restart_interval(&(++cpy_data));
					break;
                //All possible app headers from 0-F. Call decode header
                case 0xE0:
					this->app_headers.push_back(JFIF_header(&(++cpy_data)));
					break;
                case 0xE1:
                case 0xE2:
                case 0xE3:
                case 0xE4:
                case 0xE5:
                case 0xE6:
                case 0xE7:
                case 0xE8:
                case 0xE9:
                case 0xEA:
                case 0xEB:
                case 0xEC:
                case 0xED:
                case 0xEE:
                case 0xEF:
                    
                    break;
                case 0xFE:
                    //comment
					this->comments.push_back(Comment(&(++cpy_data)));
                    break;
            }
        }
		else
			cpy_data++;
    }
}

void jpeg_image::decode_huffman_tables(uint8_t **data){
	uint16_t length = ((uint16_t) *((*data)++)) << 8;
	length += *((*data)++);
	for(int i = length - 2; i > 0;){
		Huffman_table new_huffman_table = Huffman_table(data);
		uint8_t type = new_huffman_table.get_type();
		uint8_t table_id = new_huffman_table.get_table_id();
		huffman_tables[type][table_id] = new_huffman_table; 
		i -= new_huffman_table.get_length();
	}
}

void jpeg_image::decode_quantization_tables(uint8_t **data){
	uint16_t length = *((*data)++) << 8;
	length += **data;
	for(int i = length - 2; i > 0;){
		Quantization_table new_quant_table = Quantization_table(data);
		this->quantization_tables[new_quant_table.get_id()] = new_quant_table;
		i -= new_quant_table.get_length();
	}
}

void jpeg_image::setup_image() {	
	//find largest Horizontal and Vertical sampling ratio
	uint8_t largest_horz = 0;
	uint8_t largest_vert = 0;
	uint16_t *blocks_per_channel = new uint16_t[this->frame_header.get_num_chans()];
	for(int i = 0; i < this->frame_header.get_num_chans(); i++){
		struct Channel_info chan_info = *(this->frame_header.get_chan_info(i));
		largest_horz = chan_info.horz_sampling > largest_horz ? chan_info.horz_sampling : largest_horz;
		largest_vert = chan_info.vert_sampling > largest_vert ? chan_info.vert_sampling : largest_vert;
		blocks_per_channel[i] = chan_info.horz_sampling * chan_info.vert_sampling;
	}

	//Find how many rows and columns of image blocks are needed
	uint16_t horz_blocks = floor(((float) this->frame_header.get_height() / (largest_horz*8)) + 0.5);
	uint16_t vert_blocks = floor(((float) this->frame_header.get_width() / (largest_vert*8)) + 0.5);
	this->decoded_image_data = new struct Image_block*[vert_blocks];
	for(uint16_t i = 0; i < vert_blocks; i++){
		this->decoded_image_data[i] = new struct Image_block[horz_blocks];
		for(int j = 0; j < horz_blocks; j++) {
			this->decoded_image_data[i][j].components = new struct Component[this->frame_header.get_num_chans()];
			for(int k = 0; k < this->frame_header.get_num_chans(); k++){
				struct Component *component = (this->decoded_image_data[i][j].components) + k;
				component->num_data_blocks = blocks_per_channel[k];
				component->data_blocks = new uint8_t[component->num_data_blocks][8][8];
			}
		}
	}
}

void jpeg_image::entropy_decode(uint8_t **data, Scan_header header){
	/*
	uint64_t data_block_index = 0;

	uint8_t total_num_chans = this->frame_header.get_num_chans();
	uint8_t scan_num_chans = header.get_num_chans();
	uint8_t *read_order = new uint8_t[total_num_chans];
	for(uint8_t i = 0; i < total_num_chans; i++){
		struct Channel_info *frame_chan_info = this->frame_header.get_chan_info(i);
		read_order[i] = 0;
		for(uint8_t j = 0; j < scan_num_chans; j++){
			struct Chan_specifier *scan_chan_spec = header.get_chan_spec(j);
			if(frame_chan_info->id == scan_chan_spec->componentID){
				read_order[i] = frame_chan_info->horz_sampling * frame_chan_info->vert_sampling;
				break;
			}
		}
	}
	
	
	uint8_t *track_read_order = new uint8_t[total_num_chans];
	while(1){
		
	}
	*/
}

uint8_t jpeg_image::decode_dc(uint8_t data_block[8][8]) {
	return 0;
} 
