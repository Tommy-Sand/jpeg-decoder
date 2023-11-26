typedef struct {
	uint8_t **encoded_data;
	uint8_t offset;
} Data;

uint8_t read_bit(Data *d);
