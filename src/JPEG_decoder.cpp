#include "jpeg_decoder.hh"

jpeg_image::jpeg_image(){}

jpeg_image::jpeg_image(const char* path): p{path} {
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
    //Look for marker 0xFFD8, indicating start of the image
    uint64_t SOI_marker = -1;
    for(int i = 0; i < size; i++){
        if(data[i] == 0xFF && i + 1 < size && data[i+1] == 0xD8)
            SOI_marker = i;
    }

    //Look for all other markers
    for(int i = SOI_marker + 2; i < size; i++){
        if(data[i] == 0xFF && i + 1 < size && data[i+1] != 0xFF && data[i+1] != 0x00){
            switch(data[i+1]){
                case 0xC0: //SOF Sequential

                case 0xC1: //SOF Sequential extended
                case 0xC2: //SOF progressive
                case 0xC3: //Lossless
                    this->frame_header = new Frame_header(this->data + i);
                case 0xC4:
                    
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
                case 0xD9:
                    //EOI
                case 0xDA:
                    //SOS
                case 0xDB:
                    //Quantization
                    //if(quantization size == 0)
                        //make quantization table
                    //else
                        //add to quantization table
                    break;
                case 0xDD:
                    //restart interval

                //All possible app headers from 0-F. Call decode header
                case 0xE0:
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
                    //
                    break;
                case 0xFE:
                    //comment
                    break;

            }
        }


    }
}