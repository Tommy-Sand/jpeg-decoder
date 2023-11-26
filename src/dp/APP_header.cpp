#include "JPEG_decoder.hh"

APP_header::APP_header(uint8_t **data): data{*data} {
    this->length = *((*data)++) << 8;
	this->length += **data;
}

JFIF_header::JFIF_header(uint8_t **data) : APP_header(data){
	bool JFIF_marker = true;
	uint8_t JFIF[5] = {0x4A, 0x46, 0x49, 0x46, 0x00};
	for(uint8_t x: JFIF){
		if(x != *(++(*data))){
			JFIF_marker = false;
		}
	}

    this->version = *(++(*data)) << 8;
    this->version += *(++(*data));

    this->dens_units = *(++(*data));

    this->x_dens = *(++(*data)) << 8;
    this->x_dens += *(++(*data));

    this->y_dens = *(++(*data)) << 8;
    this->y_dens += *(++(*data));

    this->x_thumbnail = *(++(*data));
    this->y_thumbnail = *(++(*data));

    this->thumbnail_length = 3 * (this->x_thumbnail * this->y_thumbnail);
    this->thumbnail = ++(*data);
	*data = *data + this->thumbnail_length;
}
