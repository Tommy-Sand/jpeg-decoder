#include "JPEG_decoder.hh"

Comment::Comment(uint8_t **data) {	
    this->length = *((*data)++) << 8;
    this->length += *((*data)++);

	this->comment.append((char *) *data, this->length - 2);
}
