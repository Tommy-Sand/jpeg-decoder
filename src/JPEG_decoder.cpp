#include "jpeg_decoder.hh"

jpeg_image::jpeg_image(){}

jpeg_image::jpeg_image(const char* path): p{path} {
    assert(std::filesystem::exists(p));
    image = std::ifstream (p, std::ios::binary);
    assert(image.is_open());
    assert(image.good());

    size = std::filesystem::file_size(p);
    data = new uint8_t[size+1];

    image.read(reinterpret_cast<char*>(&data), size);
    data[size] = '\0';
    assert(!image.fail());

    image.close();
    assert(!image.bad());

    //find markers in data
    this->find_markers();
}

jpeg_image::jpeg_image(void *data, uint64_t size): data{(uint8_t *) data}, size{size} {}

