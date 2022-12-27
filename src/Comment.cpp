#include "jpeg_decoder.hh"

Comment::Comment(uint8_t *data): data{data} {
    uint16_t pos = 1;

    this->length = *(data + (++pos)) << 8;
    this->length = *(data + (++pos));

    this->comment = new char[(this->length) - 1];
    uint16_t comment_length = (this->length) - 2;
    for(int i = 0; i < comment_length; i++)
        this->comment[i] = (char) *(data + (++pos));
    
    this->comment[comment_length] = '\0';
}