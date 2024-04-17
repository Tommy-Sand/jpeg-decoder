#include "decode.h"
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

uint8_t log_du1 = 2;

Image *allocate_img(FrameHeader *fh) {
	Image *img = calloc(1, sizeof(Image));
	if (!img) {
		return NULL;
	}
	img->buf = (uint8_t **) calloc(fh->ncs, sizeof(uint8_t *));
	if (!img->buf) {
		free(img);
		return NULL;
	}
	for (uint8_t i = 0; i < fh->ncs; i++) {
		Component *c = fh->cs + i;
		uint16_t x_to_mcu = c->x + ((c->x % (8 * c->hsf)) ? (8 * c->hsf - (c->x % (8 * c->hsf))) : 0);
		uint16_t y_to_mcu = c->y + ((c->y % (8 * c->vsf)) ? (8 * c->vsf - (c->y % (8 * c->vsf))) : 0);
		/*
		printf("temp: %d\n", (8 * c->hsf));
		printf("temp: %d\n", (8 * c->vsf));
		printf("temp2: %d\n", c->y % (8 * c->hsf));
		printf("temp2: %d\n", c->y % (8 * c->vsf));
		printf("x_to_mcu: %d, y_to_mcu: %d\n", x_to_mcu, y_to_mcu);
		*/
		img->buf[i] = (uint8_t *) calloc(x_to_mcu * y_to_mcu, sizeof(uint8_t));
		if (!(img->buf[i])) {
			for (uint8_t j = 0; j < i; j++) {
				free(img->buf[j]);
			}
			free(img);
			return NULL;
		}
	}
	img->n_du = fh->ncs;
	img->mcu = 0;

	return img;
}

/*
void print_mcu(Image *img, int16_t (**mcu)[64], FrameHeader *fh) {
	for (uint8_t i = 0; i < fh->ncs; i++) {
		Component *c = fh->cs + i;
		for (uint8_t j = 0; j < c->vsf; j++) {
			for(uint8_t k = 0; k < c->hsf; k++) {
				uint16_t x_to_mcu = c->x + ((c->x % (8 * c->hsf)) ? (8 * c->hsf - (c->x % (8 * c->hsf))) : 0);
				uint16_t mcu_progress = img->mcu;
				uint16_t x = ((mcu_progress * c->hsf * 8) + (k * 8)) % x_to_mcu; 
				uint16_t y = ((((mcu_progress * c->hsf * 8) + (k * 8)) / x_to_mcu) * c->vsf * 8) + (j * 8); 
				//uint16_t y = ((((mcu_progress * c->hsf * 8) + (k * 8)) / x_to_mcu) * c->vsf * 8); 

				if (mcu_progress == 0) {
					printf("DU: %d\n", (j * c->hsf) + k);
					printf("x: %d, y: %d\n", x, y);
				}

				int16_t *du = *((*(mcu + i)) + ((j * c->hsf) + k));

				write_data_unit(img, i, x_to_mcu, du, x, y);
			}
		}
	}
	img->mcu++;
	return;

}
*/

int write_mcu(Image *img, int16_t (**mcu)[64], FrameHeader *fh) {
	for (uint8_t i = 0; i < fh->ncs; i++) {
		Component *c = fh->cs + i;
		for (uint8_t j = 0; j < c->vsf; j++) {
			for(uint8_t k = 0; k < c->hsf; k++) {
				uint16_t x_to_mcu = c->x + ((c->x % (8 * c->hsf)) ? (8 * c->hsf - (c->x % (8 * c->hsf))) : 0);
				uint16_t mcu_progress = img->mcu;
				uint16_t x = ((mcu_progress * c->hsf * 8) + (k * 8)) % x_to_mcu; 
				uint16_t y = ((((mcu_progress * c->hsf * 8) + (k * 8)) / x_to_mcu) * c->vsf * 8) + (j * 8); 
				//uint16_t y = ((((mcu_progress * c->hsf * 8) + (k * 8)) / x_to_mcu) * c->vsf * 8); 

				if (mcu_progress == 0) {
					printf("DU: %d\n", (j * c->hsf) + k);
					printf("x: %d, y: %d\n", x, y);
				}

				int16_t *du = *((*(mcu + i)) + ((j * c->hsf) + k));

				write_data_unit(img, i, x_to_mcu, du, x, y);
			}
		}
	}
	img->mcu++;
	return 0;
}

//int write_data_unit(Image *img, int16_t *du, FrameHeader *fh, uint8_t comp) {
int write_data_unit(Image *img, uint8_t comp, uint16_t x_to_mcu, int16_t *du, uint32_t x, uint32_t y) {
	for (uint16_t j = y; j < y + 8; j++) {
		for (uint16_t k = x; k < x + 8; k++) {
			*(*(img->buf + comp) + (j * x_to_mcu) + k) = (uint8_t) du[((j - y) * 8) + (k - x)];
		}
	}
	return 0;
}

int decode_scan(uint8_t **encoded_data, Image *img, FrameHeader *fh, ScanHeader *sh, HuffTables *hts, QuantTables *qts, RestartInterval ri) {
	/**
	 * For each image component in scan header order
	 * find how many 8x8 blocks for an image component
	 * Read those image components and decode
	 */
	uint8_t offset = 0;
	int16_t pred[sh->nics];
	for(uint8_t i = 0; i< sh->nics; i++) {
		pred[i] = 0;
	}
	
	uint8_t EOI = 0;
	int16_t (**mcu)[] = (int16_t (**)[]) calloc(sh->nics, sizeof(int16_t **));
	if (!mcu) {
		return -1;
	}
	for(uint8_t i = 0; i < sh->nics; i++) {
		ImageComponent ic = sh->ics[i];

		Component *c = NULL;
		for (uint8_t j = 0; j < fh->ncs; j++) {
			c = (fh->cs) + j;
			if (c->id == ic.sc) {
				break;
			}
		}
		if (c == NULL) {
			return -1;
		}
		int16_t (*dus)[64] = (int16_t (*)[64]) calloc(64 * (c->hsf * c->vsf), sizeof(int16_t));
		*(mcu + i) = dus;
	}

	//For performace measurement
	struct timespec start;
	struct timespec end;
	
	uint32_t mcus_read = 0;
	while(!EOI) {
		if (mcus_read == 24576) {
			printf("\n");
		}

		uint8_t ret = check_marker(encoded_data);
		if ((ri != 0) && ((mcus_read % ri) == 0) && ret == 1) {	
			if (ret == 1) {
				restart_marker(pred, sh->nics);
				next_byte_restart_marker(encoded_data, &offset);
			}
		}
		else if (ret == 2) {
			printf("End Reached");
			EOI = 1;
			break;
		}

		for(uint8_t i = 0; i < sh->nics; i++) {
			int16_t (*dus)[64] = *(mcu + i);
			ImageComponent ic = sh->ics[i];

			Component *c = NULL;
			for (uint8_t j = 0; j < fh->ncs; j++) {
				c = (fh->cs) + j;
				if (c->id == ic.sc) {
					break;
				}
			}
			if (c == NULL) {
				return -1;
			}
			HuffTable dc = hts->DCAC[0][ic.dc];
			HuffTable ac = hts->DCAC[1][ic.ac];
			QuantTable *qt = qts->tables + c->qtid;
			for (uint8_t j = 0; j < c->vsf; j++) {
				for (uint8_t k = 0; k < c->hsf; k++) {
					int16_t *du = *(dus + ((j * c->hsf) + k));
					memset(du, 0, 64 * sizeof(int16_t));
					if(!du) {
						return -1;
					}
					timespec_get(&start, TIME_UTC);
					if (decode_data_unit(encoded_data, &offset, du, dc, ac, pred + i) != 0) {
						free(mcu);
						return -1;
					}

					timespec_get(&end, TIME_UTC);
					uint64_t start_ns = ((uint64_t) start.tv_sec * 1000000000) + start.tv_nsec;
					uint64_t end_ns = ((uint64_t) end.tv_sec * 1000000000) + end.tv_nsec;
					uint16_t diff = end_ns - start_ns;
					printf("start: sec(%ld) nanosec(%ld)\n", start.tv_sec, start.tv_nsec);
					printf("end: sec(%ld) nanosec(%ld)\n", end.tv_sec, end.tv_nsec);
					printf("Decoding time: %dns\n", diff);
					timespec_get(&start, TIME_UTC);

					if (dequant_data_unit(qt, du) != 0) {
						free(mcu);
						return -1;
					}

					timespec_get(&end, TIME_UTC);
					start_ns = ((uint64_t) start.tv_sec * 1000000000) + start.tv_nsec;
					end_ns = ((uint64_t) end.tv_sec * 1000000000) + end.tv_nsec;
					diff = end_ns - start_ns;
					printf("start: sec(%ld) nanosec(%ld)\n", start.tv_sec, start.tv_nsec);
					printf("end: sec(%ld) nanosec(%ld)\n", end.tv_sec, end.tv_nsec);
					printf("Dequantizing time: %dns\n", diff);
					timespec_get(&start, TIME_UTC);

					fast_2didct(du); 

					timespec_get(&end, TIME_UTC);
					start_ns = ((uint64_t) start.tv_sec * 1000000000) + start.tv_nsec;
					end_ns = ((uint64_t) end.tv_sec * 1000000000) + end.tv_nsec;
					diff = end_ns - start_ns;
					printf("start: sec(%ld) nanosec(%ld)\n", start.tv_sec, start.tv_nsec);
					printf("end: sec(%ld) nanosec(%ld)\n", end.tv_sec, end.tv_nsec);
					printf("IDCT time: %dns\n", diff);
				}
			}
		}
		write_mcu(img, mcu, fh);
		mcus_read++;
	}

	if (0) {
		for (uint8_t i = 0; i < img->n_du; i++) {
			Component *c = fh->cs + i;
			uint8_t *buf = *(img->buf + i);
			printf("\n");
			//for (uint8_t j = 0; j < c->y; j++) {
			for (uint16_t j = 0; j < 1; j++) {
				printf("\n");
				for (uint16_t k = 0; k < c->x; k++) {
					//printf("%d ", *(buf + (c->x * j) + k));
					printf("%d ", *(buf + (c->x * j) + k));
				}
			}
		}
		printf("\n");
	}
	return 0;	
}

int decode_data_unit(uint8_t **encoded_data, uint8_t *offset, int16_t *du, HuffTable dc_huff, HuffTable ac_huff, int16_t *pred) {

	if (**encoded_data == 0xC7 && *(*encoded_data + 1) == 0xD2 && *(*encoded_data + 2) == 0x97) {
		printf("\n");
	}

	uint8_t *ptr = *encoded_data;

	if (*offset >= 8) {
		if (next_byte(&ptr, offset) == 1) {
			//restart_marker();
		}
	}

	int16_t curr_code = (*ptr >> (7 - ((*offset)++))) & 0x1;
	if (*offset >= 8) {
		if (next_byte(&ptr, offset) == 1) {
			//restart_marker();
		}
	}
	uint8_t mag = 0;
	for (uint8_t i = 0; i < 16; i++) {
		if (dc_huff.symbols[i] == NULL) {
			curr_code = (curr_code << 1) + ((*ptr >> (7 - ((*offset)++))) & 0x1);
			if ((*offset) >= 8) {
				next_byte(&ptr, offset);
			}
			continue;
		}

		if (dc_huff.max_codes[i] >= curr_code) {
			mag = (uint8_t) *(dc_huff.symbols[i] + (curr_code - dc_huff.min_codes[i]));
			break;
		}
		curr_code = (curr_code << 1) + ((*ptr >> (7 - ((*offset)++))) & 0x1);
		if ((*offset) >= 8) {
			next_byte(&ptr, offset);
			*offset = 0; 
		}
	}
	int16_t dc = 0;
	for (uint8_t j = 0; j < mag; j++) {
		dc = (dc << 1) + ((*ptr >> (7 - ((*offset)++))) & 0x1);
		if ((*offset) >= 8) {
			next_byte(&ptr, offset);
			(*offset) = 0; 
		}
	}

	if (dc < (1 << (mag - 1))) {
		dc += (-1 << mag) + 1;
	}

	dc += *pred;
	*pred = dc;
	du[0] = dc;


	for (uint8_t i = 1; i < 64; i++) {
		curr_code = (*ptr >> (7 - ((*offset)++))) & 0x1;
		if (*offset >= 8) {
			next_byte(&ptr, offset);
			*offset = 0; 
		}		
		for (uint8_t j = 0; j < 16; j++) {
			if (ac_huff.symbols[j] == NULL) {
				curr_code = (curr_code << 1) + ((*ptr >> (7 - ((*offset)++))) & 0x1);
				if ((*offset) >= 8) {
					next_byte(&ptr, offset);
					*offset = 0; 
				}
				continue;
			}

			if (ac_huff.max_codes[j] >= curr_code) {
				mag = (uint8_t) *(ac_huff.symbols[j] + (uint8_t) (curr_code - ac_huff.min_codes[j]));
				break;
			}
			curr_code = (curr_code << 1) + ((*ptr >> (7 - ((*offset)++))) & 0x1);
			if (*offset >= 8) {
				next_byte(&ptr, offset);
				*offset = 0; 
			}
		}	

		uint8_t size = mag % 16;
		uint8_t run = (mag >> 4) & 0xF;

		if (mag == 0xF0) {
			i += (0x10 - 1);
			continue;
		}
		else {
			i += run;
		}
		if (mag == 0x00) {
			break;
		}

		int16_t amp =  0;
		for (uint8_t j = 0; j < size; j++) {
			amp = (amp << 1) + ((*ptr >> (7 - ((*offset)++))) & 0x1);
			if (*offset >= 8) {
				next_byte(&ptr, offset);
				*offset = 0; 
			}
		}

		if (amp < (1 << (size - 1))) {
			amp += (-1 << size) + 1;
		}
		du[i] = amp;
	}

	if (0) {
		printf("\n");
		printf("decoded data block\n");
		for (uint8_t i = 0; i < 64; i++) {
			if (i % 8 == 0) {
				printf("\n");
			}
			printf("%d, ", du[i]);
		}
		printf("\n");
		log_du1--;
	}

	*encoded_data = ptr;
	return 0;
}

void fast_2didct(int16_t du[64]) {
	float du_float[64] = {0.0};
	for (uint8_t i = 0; i < 64; i++) {
		du_float[i] = (float) du[i];
	}
	for (uint8_t i = 0; i < 8; i++) {
		du_float[i] = 0.707106781 * du_float[i];
		du_float[i * 8] = 0.707106781 * du_float[i * 8];
	}

	float ret_dux[64] = {0};
	for (uint8_t i = 0; i < 8; i++) {
		fast_idct(8, ((float *) du_float) + (i * 8), ((float *) ret_dux) + (i * 8));
	}

	float in_duy[64] = {0};
	for (uint8_t j = 0; j < 8; j++) {
		in_duy[j * 8] = ret_dux[j];
		in_duy[(j * 8) + 1] = ret_dux[j + 8];
		in_duy[(j * 8) + 2] = ret_dux[j + 16];
		in_duy[(j * 8) + 3] = ret_dux[j + 24];
		in_duy[(j * 8) + 4] = ret_dux[j + 32];
		in_duy[(j * 8) + 5] = ret_dux[j + 40]; 
		in_duy[(j * 8) + 6] = ret_dux[j + 48]; 
		in_duy[(j * 8) + 7] = ret_dux[j + 56]; 
	}

	float ret_duy[64] = {0};
	for (uint8_t k = 0; k < 8; k++) {
		fast_idct(8, ((float *) in_duy) + (k * 8), ((float *) ret_duy) + (k * 8));
	}

	//transpose
	for (uint8_t j = 0; j < 8; j++) {
		du[j] = (int16_t) clamp((0.25 * ret_duy[j * 8]) + 128.0, 0.0, 255.0);
		du[j + 8] = (int16_t) clamp((0.25 * ret_duy[(j * 8) + 1]) + 128.0, 0.0, 255.0);
		du[j + 16] = (int16_t) clamp((0.25 * ret_duy[(j * 8) + 2]) + 128.0, 0.0, 255.0);
		du[j + 24] = (int16_t) clamp((0.25 * ret_duy[(j * 8) + 3]) + 128.0, 0.0, 255.0);
		du[j + 32] = (int16_t) clamp((0.25 * ret_duy[(j * 8) + 4]) + 128.0, 0.0, 255.0);
		du[j + 40] = (int16_t) clamp((0.25 * ret_duy[(j * 8) + 5]) + 128.0, 0.0, 255.0); 
		du[j + 48] = (int16_t) clamp((0.25 * ret_duy[(j * 8) + 6]) + 128.0, 0.0, 255.0); 
		du[j + 56] = (int16_t) clamp((0.25 * ret_duy[(j * 8) + 7]) + 128.0, 0.0, 255.0); 
	}
	
}

//Start with a vector of 8 inputs
void fast_idct(uint8_t len, float du[len], float ret_du[len]) {
	const float PI =  3.14159265358979323846;

	if (len == 2) {
		ret_du[0] = (float) (du[0] + du[1]);
		ret_du[1] = (float) (du[0] - du[1]);
	}
	else {
		uint8_t h_len = len / 2;
		float evens[h_len];
		float odds[h_len];
		for (uint8_t i = 0; i < h_len; i++) {
			evens[i] = du[i * 2];
			odds[i]  = du[(i * 2) + 1];
		}

		float ret_evens[h_len];
		float ret_odds[h_len];
		memset(ret_evens, 0, sizeof(float) * h_len);
		memset(ret_odds, 0, sizeof(float) * h_len);

		fast_idct(h_len, evens, ret_evens);
		fast_idct(h_len, odds, ret_odds);
		
		for (uint8_t i = 0; i < h_len; i++) {
			float temp = cos((PI * ((float) i + 0.5)) / (float) len) * ret_odds[i];
			ret_du[i] = ret_evens[i] + temp;
			ret_du[len - i - 1] = ret_evens[i] - temp;
		}
	}
}

uint8_t clamp(double in, double min, double max) {
	if (in > max) {
		return (uint8_t) max;
	}
	return (in < min) ? (uint8_t) min : (uint8_t) round(in);
}

/**
 * ret: 0 next byte
 *      1 restart marker
 *      2 byte stuffing
 */
/*
int next_byte(uint8_t **ptr) {
	uint8_t byte = **ptr;
	if (byte == 0xFF) {
		byte = *(*ptr + 1);
		if (byte == 0x00) {
			//Byte is stuffed and we can skip the 00
			*ptr = *ptr + 2;
			return 1;
		}
		else if (byte >= 0xD0 && byte <= 0xD7) {
			return 2;
		}
	}
	++(*ptr);
	return 0;
}
*/

/**
 * ret: 0 next byte
 *      1 restart marker
 *      2 byte stuffing
 */
 //TODO Handle possibility of multiple stuffed bytes in a row
int next_byte(uint8_t **ptr, uint8_t *offset) {
	uint8_t byte = **ptr;
	if (byte == 0xFF) {
		uint8_t n_byte = *(*ptr + 1);
		if (n_byte == 0x00) {
			//Byte is stuffed and we can skip the 00
			*ptr = *ptr + 2;
			*offset = 0;
			return 2;
		}
	}
	else {
		uint8_t n_byte = *(*ptr + 1);
		if (n_byte == 0xFF) {
			uint8_t nn_byte = *(*ptr + 2);
			/*
			if (nn_byte >= 0xD0 && nn_byte <= 0xD7) {
				*ptr += 3;
				*offset = 0;
				return 1;
			}
			else if (nn_byte == 0x00) {
			*/
			if (nn_byte == 0x00) {
				*ptr = *ptr + 1;
				*offset = 0;
				return 0;
			}
			//TODO handle DNL
		}
	}
	++(*ptr);
	*offset = 0;
	return 0;
}

/**
 * ret: 0 next byte
 *      1 restart marker
 *      2 byte stuffing
 */
 //TODO Handle possibility of multiple stuffed bytes in a row
int next_byte_restart_marker(uint8_t **ptr, uint8_t *offset) {
	uint8_t byte = **ptr;
	if (byte == 0xFF) {
		uint8_t n_byte = *(*ptr + 1);
		if (n_byte == 0x00) {
			//Byte is stuffed and we can skip the 00
			*ptr = *ptr + 2;
			*offset = 0;
			return 2;
		}
		if (n_byte >= 0xD0 && n_byte <= 0xD7) {
			*ptr += 2;
			*offset = 0;
			return 1;
		}
	}
	else {
		uint8_t n_byte = *(*ptr + 1);
		if (n_byte == 0xFF) {
			uint8_t nn_byte = *(*ptr + 2);
			if (nn_byte >= 0xD0 && nn_byte <= 0xD7) {
				*ptr += 3;
				*offset = 0;
				return 1;
			}
			else if (nn_byte == 0x00) {
				*ptr = *ptr + 1;
				*offset = 0;
				return 0;
			}
			//TODO handle DNL
		}
	}
	++(*ptr);
	*offset = 0;
	return 0;
}

/*
	1 is restart marker
	2 end of image
*/
uint8_t check_marker(uint8_t **ptr) {
	uint8_t byte = **ptr;
	uint8_t n_byte = *(*ptr + 1);
	if (byte == 0xFF && n_byte == 0xD9) {
		return 2;
	}

	uint8_t nn_byte = *(*ptr + 2);
	if (byte == 0xFF) {
		if (n_byte >= 0xD0 && n_byte <= 0xD7) {
			return 1;
		}

		uint8_t nnn_byte = *(*ptr + 3);
		//printf("nnn_byte: %X\n", nnn_byte);
		if (n_byte == 0x00 && nn_byte == 0xFF && (nnn_byte >= 0xD0 && nnn_byte <= 0xD7)) {
			return 1;
		}
		else if (n_byte == 0x00 && nn_byte == 0xFF && nnn_byte >= 0xD9) {
			return 2;
		}
	}
	else if (n_byte == 0xFF && (nn_byte >= 0xD0 && nn_byte <= 0xD7)) {
		return 1;
	}
	else if (n_byte == 0xFF && nn_byte == 0xD9) {
		return 2;
	}
	return 0;
}

void restart_marker(int16_t *pred, uint8_t len) {
	for(uint8_t i = 0; i < len; i++) {
		*(pred + i) = 0;
	}
}
