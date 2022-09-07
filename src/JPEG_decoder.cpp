#include "JPEG_decoder.hh"

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
                std::cout << "Quantized table found\n";
                qtable = read_QTable(&image);
                break;
            case 0xC0:
                std::cout << "BaseLine DCT found\n";
                dctheader = read_DCTheader(&image);
                break;
            case 0xC2:
                std::cout << "Progressive DCT found\n";
                break;
            case 0xC4:
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
            case 0xDA:{
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
                //Caluclate the number of MCU blocks that should be read

                Channel_info max_sampling_chan = *dctheader->chan_infos;
                for(int i = 1; i < dctheader->num_chans; i++){
                    Channel_info current_chan = *(dctheader->chan_infos + i);
                    if(max_sampling_chan.horz_sampling * max_sampling_chan.vert_sampling < max_sampling_chan.horz_sampling * max_sampling_chan.vert_sampling)
                        max_sampling_chan = current_chan;
                }
                uint16_t num_MCUs = ceil((float) dctheader->width/(max_sampling_chan.horz_sampling * 8)) * ceil((float) dctheader->height/(max_sampling_chan.vert_sampling * 8));
                for(int i = 0; i < num_MCUs; i++){
                    for(int j = 0; j < MCU_block.size(); j++){
                        for(int k = 0; k < 7; k++){
                            for(int l = 0; l < 7; l++){
                                MCU_block[j]->data[k][l] = 0;
                            }
                        }
                    }   
                    read_MCU(&image);
                    
                }
                //std::exit(0);
                break;
            }
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
        if(cur_byte != 0xFF){
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
    
    uint8_t *buffer = new uint8_t[Length - 2];
    image->read(reinterpret_cast<char*>(buffer), Length - 2);

    uint8_t Size = (buffer[0] >> 4) & 0xF;
    uint8_t ID =  buffer[0] & 0xF;

    uint16_t *Qtable = new uint16_t[64];

    for(int i = 0; i < 64; i++){
        Qtable[i] = *(buffer + 1 + i);
    }

    return new QTable(Length, Size, ID, Qtable);
}

DCTheader *read_DCTheader(std::ifstream *image){
    uint16_t Length = cur_byte;
    image->read(reinterpret_cast<char*>(&cur_byte), 1);
    Length = (Length << 8) + cur_byte;

	uint16_t Buffer_pos = 0;
    uint8_t *Buffer = new uint8_t[Length - 2];
	image->read(reinterpret_cast<char *>(Buffer), Length - 2);

    uint8_t Sample_percision = Buffer[Buffer_pos++];
    
    uint16_t Height = Buffer[Buffer_pos++];
    Height = (Height << 8) + Buffer[Buffer_pos++];

    uint16_t Width = Buffer[Buffer_pos++];
    Width = (Width << 8) + Buffer[Buffer_pos++];

    uint8_t Num_chans = Buffer[Buffer_pos++];
    
	Channel_info *Channel_infos = new Channel_info[Num_chans];
    for(uint8_t i = 0; i < Num_chans; i++){
        Channel_infos[i].identifier = Buffer[Buffer_pos++];
        Channel_infos[i].horz_sampling = (Buffer[Buffer_pos] >> 4) & 0xF;
        Channel_infos[i].vert_sampling = (Buffer[Buffer_pos++]  & 0xF);
        Channel_infos[i].qtableID = Buffer[Buffer_pos++];
    }

	free(Buffer);

    return new DCTheader(Length, Sample_percision, Height, Width, Num_chans, Channel_infos);
}

Scan_header *read_Scan_header(std::ifstream *image){
    uint16_t Length = cur_byte;
    image->read(reinterpret_cast<char*>(&cur_byte), 1);
    Length = (Length << 8) + cur_byte;

    uint8_t *Buffer = new uint8_t[Length - 2];
    image->read(reinterpret_cast<char*>(Buffer), Length - 2);

	uint8_t i = 0;
    uint8_t Num_chans = Buffer[i++];
    Chan_specifier *Chan_specs = new Chan_specifier[Num_chans];
    for(uint8_t j = 0; j < Num_chans; j++){
    	Chan_specs[j].componentID = Buffer[i++];
		Chan_specs[j].Huffman_DC = (Buffer[i] >> 4) & 0xF;
		Chan_specs[j].Huffman_AC = Buffer[i++] & 0xF;
    }
	uint8_t Spectral_start = Buffer[i++];
	uint8_t Spectral_end = Buffer[i++];
	uint8_t Successive_approx = Buffer[i++];

    free(Buffer);
    return new Scan_header(Length, Num_chans, Chan_specs, Spectral_start, Spectral_end, Successive_approx);
}

HTable *read_HTable(std::ifstream *image){
    uint16_t Length = cur_byte;
	image->read(reinterpret_cast<char*>(&cur_byte), 1);
	Length = (Length << 8) + cur_byte;

    uint16_t Buffer_pos = 0;
    uint8_t *Buffer = new uint8_t[Length - 2];
    image->read(reinterpret_cast<char*>(Buffer), Length - 2);

    uint8_t Type = (Buffer[Buffer_pos] >> 4) & 0xF;
    uint8_t Table_ID = Buffer[Buffer_pos++] & 0xF;

    uint8_t Hcodes_lengths[16];
    for(int i = 0; i < 16; i++){
        Hcodes_lengths[i] = Buffer[Buffer_pos++];
    }

    uint8_t *Coded_symbol_array = new uint8_t [Length - 19];
    for(int i = 0; i < Length - 19; i++){
        Coded_symbol_array[i] = Buffer[Buffer_pos++];
    }
    
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
            min_symbol[i] = (max_symbol[last_non_zero_length] + 1) * (1 << (i - last_non_zero_length));
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
    int16_t Size = 0;
    uint8_t Size_read = 0;
    int16_t Value = 0;
    bool skip_byte = false;

    while(!image->eof() && Size_read < 12){
        if(pos < 0){
            feed_buffer(image);
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
            else{
                std::cout << "ERROR\n";
                exit(1);
            }
        }
    }


    uint8_t i = 0;
    while(i++ < Size){
        if(pos < 0){
            feed_buffer(image);
        }
        Value = (Value << 1) + ((cur_byte >> pos--) & 1);
    }

    if(Value < (1 << (Size - 1))){
        int difference = 2 * ((1 << (Size - 1)) - Value) - 1;
        Value = - (Value + difference);
    }
    return Value;
}

AC_coefficient *decode_AC_coefficient(std::ifstream *image, HTable *htable){
    int16_t RLength_or_Size = 0;
    uint8_t RLength_or_Size_read = 0;
    int16_t Value = 0;
    bool skip_byte = false;

    while(!image->eof() && RLength_or_Size_read < 16){
        if(pos < 0){
            feed_buffer(image);
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
            else{
                std::cout << "ERROR\n";
                exit(1);
            }
        }
    }
    
    if(RLength_or_Size == 0)
        return new AC_coefficient(true, 0, 0);
    uint8_t Size = RLength_or_Size & 0xF;
    uint8_t RLength = (RLength_or_Size >> 4) & 0xF;

    if(Size == 0){
        return new AC_coefficient(false, 15, 0);
    }

    uint8_t i = 0;
    while(!image->eof() && i++ < Size){
        if(pos < 0){
            feed_buffer(image);
        }
        Value = (Value << 1) + ((cur_byte >> pos--) & 1);
    }

    if(Value < (1 << (Size - 1))){
        int difference = 2 * ((1 << (Size - 1)) - Value) - 1;
        Value = - (Value + difference);
    }
    return new AC_coefficient(false, RLength, Value);
}

void feed_buffer(std::ifstream *image){
    if(skip_byte){
            image->read(reinterpret_cast<char*>(&cur_byte), 1);
            skip_byte = false;
    }
    image->read(reinterpret_cast<char*>(&cur_byte), 1);
    pos = 7;
    if(cur_byte == 0xFF){
        uint8_t next_byte = image->peek();
        if(next_byte == 0x00){
            skip_byte = true;
        }
        else if(next_byte == 0xD9){
            std::cout << "ERROR: END OF FILE" << std::endl;
            exit(1);
        }
        else if(next_byte >= 0xD0 && next_byte <= 0xD7){
            std::cout << "Restart" << std::endl;
        }
    }
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
		for(int j = 0; j < MCU[i]; j++){
			read_data_block(MCU_block[block_pos++], image, i);
        }
	}
}

void read_data_block(Data_block *data_block, std::ifstream *image, uint8_t id){
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

	data_block->data[0][0] = pred + decode_DC_coefficient(image, htables[i]);
    pred = data_block->data[0][0];
	uint8_t coeff_count = 1;
	while(coeff_count < 64){
		AC_coefficient *AC = decode_AC_coefficient(image, htables[j]);
		if(AC->EOB){
			return;
        }
		else{
			coeff_count += AC->run_length;
			uint8_t decoded_horz = (zigzag[coeff_count] >> 4) & 0xF;
			uint8_t decoded_vert = (zigzag[coeff_count]) & 0xF;
			data_block->data[decoded_vert][decoded_horz] = AC->value;
		}
	}

}