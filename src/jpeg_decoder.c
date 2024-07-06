#include <SDL2/SDL.h>
#include <stdbool.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#include "decode.h"
#include "frame_header.h"
#include "huff_table.h"
#include "quant_table.h"
#include "restart_interval.h"
#include "scan_header.h"


int display_image(int width, int height, SDL_Surface* image);
int read_app_segment(uint8_t** encoded_data);
int mmap_file(const char *filename, uint8_t **data);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Need file path\n");
        return -1;
    }

	uint8_t *buf;
	size_t size = mmap_file(argv[1], &buf);
	if (size == -1) {
		return -1;
	}
	printf("length: %lu\n", size);

    //switch statement for markers
    FrameHeader* fh = new_frame_header();
    if (fh == NULL) {
        printf("Cannot malloc fh");
        return -1;
    }
    fh->process = BDCT;

    ScanHeader sh;
    QuantTables* qts = NULL;
    HuffTables* hts = NULL;
    Image* img = NULL;
    RestartInterval ri = 0;

    for (uint8_t* ptr = buf; ptr < buf + size;) {
        bool EOI = false;
        if (*(ptr++) == 0xFF) {
            switch (*(ptr++)) {
                case 0x01:  //Temp private use in arithmetic coding
                //Markers from
                case 0x02:
                //Through
                case 0xBF:
                    //Are Reserved
                    break;

                case 0xC0:
                    printf("DEBUG: SOF Baseline DCT\n");
                    if (fh == NULL) {
                        //malloc fail check in decode_frame_header
                        fh = new_frame_header();
                        if (fh == NULL) {
                            printf("Cannot malloc fh");
                            return -1;
                        }
                    }
                    if (decode_frame_header(BDCT, &ptr, fh) == -1) {
                        printf("DEBUG: frame header read failed\n");
                        return -1;
                    }
                    print_frame_header(fh);
                    if (img == NULL) {
                        img = allocate_img(fh);
                        if (!img) {
                            printf("Cannot allocate image");
                            return -1;
                        }
                    }
                    break;
                case 0xC1:  //SOF Extended Sequential DCT
                    printf("DEBUG: SOF Extended Sequential DCT\n");
                    break;
                case 0xC2:  //SOF Progressive DCT
                    printf("DEBUG: SOF Progressive DCT\n");
                    break;
                case 0xC3:  //SOF Lossless Sequential
                    printf("DEBUG: SOF Lossless DCT\n");
                    break;
                case 0xC5:  //SOF Differential sequential DCT
                    printf("DEBUG: SOF Differential Sequential DCT\n");
                    break;
                case 0xC6:  //SOF Differential progressive DCT
                    printf("DEBUG: SOF Differential Progressive DCT\n");
                    break;
                case 0xC7:  //SOF Differential lossless (sequential)
                    printf("DEBUG: SOF Differential lossless DCT\n");
                    break;

                case 0xC8:  //SOF Reserved for JPEG extensions
                    printf("DEBUG: SOF Reserved for JPEG extensions\n");
                    break;
                case 0xC9:  //SOF Extended sequential DCT
                    printf("DEBUG: SOF Reserved for JPEG extensions\n");
                    break;
                case 0xCA:  //SOF Progressive DCT
                    printf("DEBUG: SOF Progressive DCT\n");
                    break;
                case 0xCB:  //SOF Lossless (sequential)
                    printf("SOF Lossless (sequential)\n");
                    break;

                case 0xCD:  //SOF Differential sequential DCT
                    printf("DEBUG: SOF Differential sequential DCT\n");
                    break;
                case 0xCE:  //SOF Differential progressive DCT
                    printf("DEBUG: SOF Differential progressive DCT\n");
                    break;
                case 0xCF:  //SOF Differential lossless (sequential)
                    printf("DEBUG: SOF Differential lossless\n");
                    break;

                case 0xC4:  //Define Huffman table(s)
                    printf("DEBUG: Define Huffman Table(s)\n");
                    if (fh == NULL) {
                        return -1;
                    }
                    if (hts == NULL) {
                        hts = new_huff_tables(fh->process);
                        if (hts == NULL) {
                            printf("Cannot malloc hts");
                            return -1;
                        }
                    }
                    if (decode_huff_tables(&ptr, hts) != 0) {
                        printf("DEBUG: Huff Table read failed\n");
                        return -1;
                    }
                    break;

                case 0xCC:  //Define arithmetic coding conditioning(s)
                    printf("DEBUG: Define Arithmetic coding conditioning(s)\n");
                    break;

                //For D0-D7 Restart with modulo 8 count “m”
                case 0xD0:
                case 0xD1:
                case 0xD2:
                case 0xD3:
                case 0xD4:
                case 0xD5:
                case 0xD6:
                case 0xD7:
                    printf("DEBUG: Restart marker\n");
                    break;
                case 0xD8:  //SOI
                    printf("DEBUG: SOI\n");
                    break;
                case 0xD9:  //EOI
                    printf("DEBUG: EOI\n");
                    //return 0;
                    EOI = true;
                    break;
                case 0xDA:  //SOS
                    printf("DEBUG: SOS\n");

                    if (decode_scan_header(&ptr, &sh) != 0) {
                        printf("DEBUG: Scan Header read failed\n");
                        return -1;
                    }

                    print_scan_header(&sh);
                    if (decode_scan(&ptr, img, fh, &sh, hts, qts, ri) != 0) {
                        printf("DEBUG: Decode Scan failed\n");
                        return -1;
                    }
                    break;
                case 0xDB:  //Define Quant Table
                    printf("DEBUG: Definition of quant table\n");
                    if (qts == NULL) {
                        qts = new_quant_tables();
                        if (qts == NULL) {
                            printf("Cannot allocate memory for qts");
                            return -1;
                        }
                    }
                    if (decode_quant_table(&ptr, qts) == -1) {
                        printf("DEBUG: Quant Table read failed\n");
                    }
                    break;
                case 0xDC:  //Define number of lines
                    printf("DEBUG: Defined Number of lines\n");
                    if (fh == NULL) {
                        printf("DEBUG: fh is null when reading DNL");
                    }
                    if (decode_number_of_lines(&ptr, fh) != 0) {
                        printf("DEBUG: Number of Lines read fialed\n");
                    }
                    break;
                case 0xDD:  //Define restart interval
                    printf("DEBUG: Define restart interval\n");
                    if (decode_restart_interval(&ptr, &ri) != 0) {
                        printf("DEBUG: Restart Interval read failed\n");
                        return -1;
                    }

                    break;
                case 0xDE:  //Define Hierarchical Progression
                    printf("DEBUG: Hierarchical progression\n");
                    break;
                case 0xDF:  //Expand reference components
                    printf("DEBUG: Expand reference components\n");
                    break;

                //For E0-EF Reserved for Application segment
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
                    printf("DEBUG: APPLICATION Segment\n");
                    read_app_segment(&ptr);
                    break;

                //For F0-FD Reserved for JPEG extensions
                case 0xF0:
                case 0xF1:
                case 0xF2:
                case 0xF3:
                case 0xF4:
                case 0xF5:
                case 0xF6:
                case 0xF7:
                case 0xF8:
                case 0xF9:
                case 0xFA:
                case 0xFB:
                case 0xFC:
                case 0xFD:
                    printf("DEBUG: Reserved JPEG extensions\n");
                    break;

                case 0xFE:  //Comment
                    printf("DEBUG: Comment\n");
                    break;
            }
        }

        if (EOI) {
            break;
        }
    }

    print_huff_tables(hts);

    if (!fh) {
        return -1;
    }
    uint16_t width = fh->X;
    uint16_t height = fh->Y;

    if (SDL_Init(SDL_INIT_TIMER)) {
        printf("SDL init failed\n");
        return -1;
    }

    SDL_Surface* img_surface = SDL_CreateRGBSurfaceWithFormat(
        0,
        width,
        height,
        24,
        SDL_PIXELFORMAT_RGB24
    );
    if (!img_surface) {
        printf("failed\n");
        return -1;
    }
    uint16_t pitch = img_surface->pitch;
    width = img_surface->w;
    height = img_surface->h;
    uint8_t* pixels = img_surface->pixels;

    for (uint32_t i = 0; i < height * pitch; i++) {
        *(pixels + i) = 0;
    }

    uint8_t max_hsf = 0;
    uint8_t max_vsf = 0;
    for (uint8_t i = 0; i < fh->ncs; i++) {
        Component c = *(fh->cs + i);
        if (c.hsf > max_hsf) {
            max_hsf = c.hsf;
        }
        if (c.vsf > max_vsf) {
            max_vsf = c.vsf;
        }
    }

    if (fh->ncs == 1) {
        for (uint32_t j = 0; j < width * height; j++) {
            pixels[j * 3] = *((*img->buf) + j);
            pixels[(j * 3) + 1] = *((*img->buf) + j);
            pixels[(j * 3) + 2] = *((*img->buf) + j);
        }
    }
    if (fh->ncs == 3) {
        Component c = *fh->cs;
        float hratio0 = (float)c.hsf / max_hsf;
        float vratio0 = (float)c.vsf / max_vsf;
        uint16_t x_to_mcu0 =
            c.x + ((c.x % (8 * c.hsf)) ? (8 * c.hsf - (c.x % (8 * c.hsf))) : 0);

        c = *(fh->cs + 1);
        float hratio1 = (float)c.hsf / max_hsf;
        float vratio1 = (float)c.vsf / max_vsf;
        uint16_t x_to_mcu1 =
            c.x + ((c.x % (8 * c.hsf)) ? (8 * c.hsf - (c.x % (8 * c.hsf))) : 0);

        c = *(fh->cs + 2);
        float hratio2 = (float)c.hsf / max_hsf;
        float vratio2 = (float)c.vsf / max_vsf;
        uint16_t x_to_mcu2 =
            c.x + ((c.x % (8 * c.hsf)) ? (8 * c.hsf - (c.x % (8 * c.hsf))) : 0);

        for (uint32_t j = 0; j < height; j++) {
            for (uint32_t k = 0; k < width; k++) {
                uint8_t Y =
                    *(*(img->buf)
                      + (((uint32_t)(j * vratio0) * x_to_mcu0)
                         + (uint32_t)(hratio0 * k)));
                uint8_t Cb =
                    *(*(img->buf + 1)
                      + (((uint32_t)(j * vratio1) * x_to_mcu1)
                         + (uint32_t)(hratio1 * k)));
                uint8_t Cr =
                    *(*(img->buf + 2)
                      + (((uint32_t)(j * vratio2) * x_to_mcu2)
                         + (uint32_t)(hratio2 * k)));

                float R = Y + 1.402 * (((float)Cr) - 128.0);
                float G = Y - 0.34414 * (((float)Cb) - 128.0)
                    - 0.71414 * (((float)Cr) - 128.0);
                float B = Y + 1.772 * (((float)Cb) - 128.0);

                pixels[j * pitch + (k * 3)] =
                    (R < 0.0) ? 0 : ((R > 256.0) ? 256 : R);
                pixels[(j * pitch) + (k * 3) + 1] =
                    (G < 0.0) ? 0 : ((G > 256.0) ? 256 : G);
                pixels[(j * pitch) + (k * 3) + 2] =
                    (B < 0.0) ? 0 : ((B > 256.0) ? 256 : B);
            }
        }
    }
    free_img(img);

    display_image(width, height, img_surface);
    SDL_FreeSurface(img_surface);
    SDL_Quit();

    munmap(buf, size);
    if (fh != NULL) {
        free_frame_header(fh);
    }
    if (qts != NULL) {
        free_quant_tables(qts);
    }
    if (hts != NULL) {
        free_huff_tables(hts);
    }
    return 0;
}

int display_image(int width, int height, SDL_Surface* image) {
    SDL_Window* window =
        SDL_CreateWindow("Test window", 100, 100, width, height, 0);
    if (window == NULL) {
        printf("SDL create window failed\n");
        return -1;
    }

    SDL_Surface* surface = SDL_GetWindowSurface(window);
    if (SDL_BlitSurface(image, NULL, surface, NULL)) {
        printf("failure");
        return -1;
    }
    SDL_UpdateWindowSurface(window);

    SDL_Event e;
    bool quit = false;
    while (quit == false) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }
    }
    return 0;
}

int mmap_file(const char* filename, uint8_t** data) {
    struct stat st;
    if (stat(filename, &st) == -1) {
        fprintf(stderr, "Could not get size of file, %s", strerror(errno));
        return -1;
    }
    off_t len = st.st_size;

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Could not open file, %s", strerror(errno));
        return -1;
    }

    void* mmap_ret = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, 0);
    if (mmap_ret == (void*)-1) {
        fprintf(stderr, "Could not get contents of file, %s", strerror(errno));
        return -1;
    }
    *data = mmap_ret;

    if (close(fd) == -1) {
        fprintf(stderr, "Could not close file descriptor, %s", strerror(errno));
        return -1;
    };
    return len;
}
