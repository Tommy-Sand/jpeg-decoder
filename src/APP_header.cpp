#include "JPEG_decoder.hh"

APP_header::APP_header(uint8_t *data): data{data} {
    this->length = (*(data + 2) << 8) + *(data + 3);
}

JFIF_header::JFIF_header(uint8_t *data) : APP_header(data){
    uint16_t pos = 1;

    pos += 5;

    this->version = *(data + (++pos)) << 8;
    this->version += *(data + (++pos));

    this->dens_units = *(data + (++pos));

    this->x_dens = *(data + (++pos)) << 8;
    this->x_dens += *(data + (++pos));

    this->y_dens = *(data + (++pos)) << 8;
    this->y_dens += *(data + (++pos));

    this->x_thumbnail = *(data + (++pos));
    this->y_thumbnail = *(data + (++pos));

    this->thumbnail_length = 3 * (this->x_thumbnail * this->y_thumbnail);
    this->thumbnail = data + (++pos);
}
