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
					this->convert_RGB();
					break;
                case 0xDA:
                    //SOS
					this->scan_headers.push_back(Scan_header(&(++cpy_data)));
					this->entropy_decode(this->scan_headers[this->scan_headers.size() - 1], &(++cpy_data));
					cpy_data--;
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
		Huffman_table* new_huffman_table = new Huffman_table(data);
		uint8_t type = new_huffman_table->get_type();
		uint8_t table_id = new_huffman_table->get_table_id();
		huffman_tables[type][table_id] = new_huffman_table; 
		i -= new_huffman_table->get_length();
	}
}

void jpeg_image::decode_quantization_tables(uint8_t **data){
	uint16_t length = *((*data)++) << 8;
	length += **data;
	for(int i = length - 3; i > 0;){
		Quantization_table* quant_table = new Quantization_table(data);
		this->quantization_tables[quant_table->get_id()] = quant_table;
		i -= quant_table->get_length();
	}
	*(++(*data));
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
	this->image_block_size.X = horz_blocks;
	this->image_block_size.Y = vert_blocks;
	this->decoded_image_data = new struct Image_block*[vert_blocks];
	for(uint16_t i = 0; i < vert_blocks; i++){
		this->decoded_image_data[i] = new struct Image_block[horz_blocks];
		for(int j = 0; j < horz_blocks; j++) {
			this->decoded_image_data[i][j].components = new struct Component[this->frame_header.get_num_chans()];
			for(int k = 0; k < this->frame_header.get_num_chans(); k++){
				struct Component *component = (this->decoded_image_data[i][j].components) + k;
				component->num_data_blocks = blocks_per_channel[k];
				component->data_blocks = new int16_t[component->num_data_blocks][8][8]{{{0}}};
			}
		}
	}
}

void jpeg_image::entropy_decode(Scan_header header, uint8_t **cpy_data){
	uint64_t data_block_index = 0;
	bool EOS = false;
	uint8_t total_num_chans = this->frame_header.get_num_chans();
	uint8_t scan_num_chans = header.get_num_chans();
	int16_t *pred = new int16_t[total_num_chans]{0}; 
	//Build a read order array
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
	
	

	byte = *((*cpy_data)++);	
	uint32_t max_index = this->image_block_size.X * this->image_block_size.Y;
	uint32_t index = 0;
	for(; EOS == false && index < max_index ; index++){
		struct Image_block image_block = this->get_image_block(index);
		for(int i = 0; i < total_num_chans; i++){
			Quantization_table* quant_table = this->get_quantization_table(this->frame_header.get_chan_info(i)->qtableID);
			struct Component component = image_block.components[i];
			Huffman_table* huff_DC;
			Huffman_table* huff_AC;
			uint8_t chan_id = this->frame_header.get_chan_info(i)->id;
			for(int j = 0; j < scan_num_chans; j++){
				struct Chan_specifier *chan_spec = header.get_chan_spec(j);
				if(chan_spec->componentID != chan_id)
					continue;
				else{
					huff_DC = this->get_huffman_table(0, chan_spec->Huffman_DC);	
					huff_AC = this->get_huffman_table(1, chan_spec->Huffman_AC);
					break;
				}
			}
			for(int j = 0; j < read_order[i]; j++){
				if(this->decode_dc(component.data_blocks[j], cpy_data, huff_DC, pred + i, &EOS) == 1)
					return;
				if(this->decode_ac(component.data_blocks[j], cpy_data, huff_AC, &EOS) == 1)
					return;
				quant_table->dequantize(component.data_blocks[j]);
				this->IDCT(component.data_blocks[j]);
			}
		}
	}
	delete[] pred;
	delete[] read_order;
	std::cout << "Done with this scan" << std::endl;
}

uint8_t jpeg_image::decode_dc(int16_t data_block[8][8], uint8_t **cpy_data, Huffman_table* huff, int16_t *pred, bool *EOS) {
	uint8_t code_length = 0;
	uint16_t code = 0;
	uint8_t size = 0; 
	int16_t symbol = 0;
	int32_t *min_code_of_len = huff->get_min_code_values();
	int32_t *max_code_of_len = huff->get_max_code_values();
	uint8_t **symbols = huff->get_symbol_arrays(); 
	do{
		code = (code << 1) + get_bit(cpy_data, EOS);
		if(*EOS == true)
			return 1;
		code_length++;
		if(code <= max_code_of_len[code_length - 1]){
			uint16_t code_index = code - min_code_of_len[code_length - 1];
			size = symbols[code_length - 1][code_index];
			break;
		}
	}while(code_length < 12);
	
	for(int i = 0; i < size; i++)
		symbol = (symbol << 1) + get_bit(cpy_data, EOS);
	int16_t value = coefficient_decoding(symbol, size);
	data_block[0][0] = *pred + value;
	*pred = data_block[0][0];

	return 0;
}

uint8_t jpeg_image::decode_ac(int16_t data_block[8][8], uint8_t **cpy_data, Huffman_table* huff, bool *EOS){
	int32_t *min_code_of_len = huff->get_min_code_values();
	int32_t *max_code_of_len = huff->get_max_code_values();
	uint8_t **symbols = huff->get_symbol_arrays(); 

	for(int i = 1; i <= 64; i++){
		uint8_t code_length = 0;
		uint16_t code = 0;
		uint8_t symbol = 0;
		uint8_t runlength = 0;
		uint8_t size = 0;
		int16_t value = 0;

		do{
			code = (code << 1) + get_bit(cpy_data, EOS);
			if(*EOS == true)
				return 1;
	
			code_length++;
			if(code <= max_code_of_len[code_length - 1]){
				uint16_t code_index = code - min_code_of_len[code_length - 1];
				symbol = symbols[code_length - 1][code_index];
				if(symbol == 0x0)
					return 0;
				runlength = (symbol >> 4) & 0xF;
				size = symbol & 0xF;
				break;
			}
		}while(code_length <= 16);
		
		for(int j = 0; j < size; j++)
			value = (value << 1) + get_bit(cpy_data, EOS);
		value = this->coefficient_decoding(value, size); 
		
		i += runlength;
		uint8_t vert = zigzag[i] & 0xF;
		uint8_t horz = (zigzag[i] >> 4) &0xF;
		data_block[vert][horz] = value;
	}

	return 0;
}

uint8_t jpeg_image::get_bit(uint8_t **cpy_data, bool *EOS){
	if(pos == 8){ //get new byte
		byte = *((*cpy_data)++);
		pos = 0;
		if(byte == 0xFF && **cpy_data == 0x00)
			(*cpy_data)++;
		else if(byte == 0xFF && **cpy_data != 0x00)
			*EOS = true;
	}
	return (byte >> (7 - (pos++))) & 0x1;	
}
	
struct Image_block jpeg_image::get_image_block(uint32_t index){
	uint32_t horz = index % this->image_block_size.X;
	uint32_t vert = index / this->image_block_size.X;
	return this->decoded_image_data[vert][horz];
}

int16_t jpeg_image::coefficient_decoding(uint16_t symbol, uint8_t size){
	if(size == 0)
		return 0;
	uint8_t sign = symbol >> (size - 1) & 1;
	if(sign == 0){ //negative 
		int16_t base = - (1 << size) + 1;
		return base + symbol;
	}
	return symbol;
}

void jpeg_image::IDCT(int16_t data_block[8][8]){
	//long double pi = 3.14159265358979323846264338327950288419716939937510;
	long double pi = 3.14;
	int16_t base_data_block[8][8];
	for(int i = 0; i < 8; i++){
		for(int j = 0; j < 8; j++){
			base_data_block[i][j] = data_block[i][j];
		}
	}

	for(int y = 0; y < 8; y++){
		for(int x = 0; x < 8; x++){
			long double sumu = 0;
            for(int u = 0; u < 8; u++){
            	long double sumv = 0;
                for(int v = 0; v < 8; v++){
                    long double alphau = (u == 0)? 1/std::sqrt(2): 1;
                    long double alphav = (v == 0)? 1/std::sqrt(2): 1;
                    long double cosx = std::cos(((2*x+1)*u*pi)/16);
                    long double cosy = std::cos(((2*y+1)*v*pi)/16);
                    long double total = alphau*alphav*base_data_block[u][v]*cosx*cosy;
                    sumv += total;
                }
                sumu += sumv;
            }
            data_block[y][x] = ((0.25) * (sumu)) + 128;
            if(data_block[y][x] > 255){
                data_block[y][x] = 255;
            }
            else if(data_block[y][x] < 0){
                data_block[y][x] = 0;
            }
        }
    }
}

void jpeg_image::convert_RGB(){
	uint16_t height = this->frame_header.get_height();
	uint16_t width = this->frame_header.get_width();
	RGB_pixel_data = new struct RGB[this->frame_header.get_height() * this->frame_header.get_width()];
	//If num_chans == 1 Black and white If num _chans == 3 YCBCR
	uint8_t num_chans = this->frame_header.get_num_chans();
	uint16_t horz_block_width = 0;
	uint16_t vert_block_width = 0;
	for(int i = 0; i < num_chans; i++){
		struct Channel_info *chan = this->frame_header.get_chan_info(i);
		horz_block_width = (horz_block_width > chan->horz_sampling) ? horz_block_width : chan->horz_sampling;
		vert_block_width = (vert_block_width > chan->vert_sampling) ? vert_block_width : chan->vert_sampling;
	}
	struct YCBCR **temp_YCBCR_pixel_block = new struct YCBCR*[vert_block_width * 8];
	for(int i = 0; i < vert_block_width * 8; i++)
		temp_YCBCR_pixel_block[i] = new struct YCBCR[horz_block_width * 8];

	uint32_t unit_index = 0;
	uint8_t pos = 0;

	for(uint32_t block_index_y = 0; block_index_y < this->image_block_size.Y; block_index_y++){
		for(uint32_t block_index_x = 0; block_index_x < this->image_block_size.X; block_index_x++){
			struct Image_block image_block = this->decoded_image_data[block_index_y][block_index_x];
			for(uint32_t pos_y = 0; pos_y < vert_block_width * 8; pos_y++){
				for(uint32_t pos_x = 0; pos_x < horz_block_width * 8; pos_x++){
					for(uint8_t comp_index = 0; comp_index < num_chans; comp_index++){
						struct Channel_info *chan_info = this->frame_header.get_chan_info(comp_index);
						struct Component component = image_block.components[comp_index];
						double ratio_x = (double) pos_x / (horz_block_width * 8);
						double ratio_y = (double) pos_y / (vert_block_width * 8);
						
						uint32_t rel_pos_x = (uint32_t) (ratio_x * (chan_info->horz_sampling * 8));
						uint32_t rel_pos_y = (uint32_t) (ratio_y * (chan_info->vert_sampling * 8));

						uint8_t index_x = (uint8_t) (ratio_x * chan_info->horz_sampling);
						uint8_t index_y = (uint8_t) (ratio_y * chan_info->vert_sampling);

						int16_t (*data_block)[8] = component.data_blocks[index_y * chan_info->vert_sampling + index_x];
						if(comp_index == 0)
							temp_YCBCR_pixel_block[pos_y][pos_x].Y = data_block[rel_pos_y % 8][rel_pos_x % 8];
						else if(comp_index == 1)
							temp_YCBCR_pixel_block[pos_y][pos_x].CB = data_block[rel_pos_y % 8][rel_pos_x % 8];
						else if(comp_index == 2)
							temp_YCBCR_pixel_block[pos_y][pos_x].CR = data_block[rel_pos_y % 8][rel_pos_x % 8];
						
					}
				}
			}
			//start here
			for(uint32_t pos_y = 0; pos_y < vert_block_width * 8; pos_y++){
				for(uint32_t pos_x = 0; pos_x < horz_block_width * 8; pos_x++){
					if(((block_index_y * vert_block_width * 8) + pos_y) >= this->frame_header.get_height() || ((block_index_x * horz_block_width * 8) + pos_x) >= this->frame_header.get_width())
						continue;
					struct YCBCR pixel = temp_YCBCR_pixel_block[pos_y][pos_x];
					uint32_t index = (((block_index_y * vert_block_width * 8) + pos_y) * this->frame_header.get_width()) + (block_index_x * horz_block_width * 8) + pos_x;
					struct RGB *rgb_pixel = &(RGB_pixel_data[index]);
					int32_t r = (int32_t) pixel.Y + 1.402 * ((int32_t) pixel.CR - 128);
					rgb_pixel->R = (uint8_t) r;
					if(r > 255) rgb_pixel->R = 255;
					if(r < 0) rgb_pixel->R = 0;
					int32_t g = (int32_t) pixel.Y - 0.344136 * ((int32_t) pixel.CB - 128) - 0.71136 * ((int32_t) pixel.CR - 128);
					rgb_pixel->G = (uint8_t) g;
					if(g > 255) rgb_pixel->G =255;
					if(g < 0) rgb_pixel->G = 0;
					int32_t b = (int32_t) pixel.Y + 1.772 * ((int32_t) pixel.CB - 128);
					rgb_pixel->B = (uint8_t) b;
					if(b > 255) rgb_pixel->B = 255;
					if(b < 0) rgb_pixel->B = 0;
				}
			}
		}
	}
	for(uint32_t i = 0; i < 1; i++){
		for(uint32_t j = 0; j < width; j++){
			struct RGB rgb = RGB_pixel_data[i*width + j];
			std::cout << "(" << (int) rgb.R << ", " << (int) rgb.G << ", " << (int) rgb.B << ")";
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
	
	for(int i = 0; i < vert_block_width * 8; i++)
		delete[] temp_YCBCR_pixel_block[i];
	delete[] temp_YCBCR_pixel_block;
}

jpeg_image::~jpeg_image(){
	for(int i = 0; i < 2; i++){
		for(int j = 0; j < 4; j++){
			delete huffman_tables[i][j];
		}
	}

	for(int i = 0; i < this->image_block_size.Y; i++){
		for(int j = 0; j < this->image_block_size.Y; j++){
			for(int k = 0; k < this->frame_header.get_num_chans(); k++){
				delete[] this->decoded_image_data[i][j].components[k].data_blocks;
			}
			delete[] this->decoded_image_data[i][j].components;
		}
		delete[] this->decoded_image_data[i];
	}
	for(int i = 0; i < 4; i++)
		delete quantization_tables[i];

	delete[] this->decoded_image_data;

	delete[] this->data;
}
