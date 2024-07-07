#include <SDL2/SDL.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "decode.h"
#include "frame_header.h"
#include "huff_table.h"
#include "quant_table.h"
#include "restart_interval.h"
#include "scan_header.h"

int display_image(int width, int height, SDL_Surface *image);
int read_app_segment(uint8_t **encoded_data);
int mmap_file(const char *filename, uint8_t **data);

int main(int argc, char *argv[]) {
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

    Image *img = NULL;
    FrameHeader fh;
    if (decode_jpeg_buffer(buf, size, &fh, &img) != 0) {
        printf("DEBUG: Decoding jpeg buffer failed\n");
        return -1;
    };

    uint16_t width = fh.X;
    uint16_t height = fh.Y;

    if (SDL_Init(SDL_INIT_TIMER)) {
        printf("SDL init failed\n");
        return -1;
    }

    SDL_Surface *img_surface = SDL_CreateRGBSurfaceWithFormat(
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
    uint8_t *pixels = img_surface->pixels;

    for (uint32_t i = 0; i < height * pitch; i++) {
        *(pixels + i) = 0;
    }

    uint8_t max_hsf = 0;
    uint8_t max_vsf = 0;
    for (uint8_t i = 0; i < fh.ncs; i++) {
        Component c = *(fh.cs + i);
        if (c.hsf > max_hsf) {
            max_hsf = c.hsf;
        }
        if (c.vsf > max_vsf) {
            max_vsf = c.vsf;
        }
    }

    if (fh.ncs == 1) {
        for (uint32_t j = 0; j < width * height; j++) {
            pixels[j * 3] = *((*img->buf) + j);
            pixels[(j * 3) + 1] = *((*img->buf) + j);
            pixels[(j * 3) + 2] = *((*img->buf) + j);
        }
    }
    if (fh.ncs == 3) {
        Component c = *fh.cs;
        float hratio0 = (float) c.hsf / max_hsf;
        float vratio0 = (float) c.vsf / max_vsf;
        uint16_t x_to_mcu0 =
            c.x + ((c.x % (8 * c.hsf)) ? (8 * c.hsf - (c.x % (8 * c.hsf))) : 0);

        c = *(fh.cs + 1);
        float hratio1 = (float) c.hsf / max_hsf;
        float vratio1 = (float) c.vsf / max_vsf;
        uint16_t x_to_mcu1 =
            c.x + ((c.x % (8 * c.hsf)) ? (8 * c.hsf - (c.x % (8 * c.hsf))) : 0);

        c = *(fh.cs + 2);
        float hratio2 = (float) c.hsf / max_hsf;
        float vratio2 = (float) c.vsf / max_vsf;
        uint16_t x_to_mcu2 =
            c.x + ((c.x % (8 * c.hsf)) ? (8 * c.hsf - (c.x % (8 * c.hsf))) : 0);

        for (uint32_t j = 0; j < height; j++) {
            for (uint32_t k = 0; k < width; k++) {
                uint8_t Y =
                    *(*(img->buf)
                      + (((uint32_t) (j * vratio0) * x_to_mcu0)
                         + (uint32_t) (hratio0 * k)));
                uint8_t Cb =
                    *(*(img->buf + 1)
                      + (((uint32_t) (j * vratio1) * x_to_mcu1)
                         + (uint32_t) (hratio1 * k)));
                uint8_t Cr =
                    *(*(img->buf + 2)
                      + (((uint32_t) (j * vratio2) * x_to_mcu2)
                         + (uint32_t) (hratio2 * k)));

                float R = Y + 1.402 * (((float) Cr) - 128.0);
                float G = Y - 0.34414 * (((float) Cb) - 128.0)
                    - 0.71414 * (((float) Cr) - 128.0);
                float B = Y + 1.772 * (((float) Cb) - 128.0);

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
    free_frame_header(&fh);
    return 0;
}

int display_image(int width, int height, SDL_Surface *image) {
    SDL_Window *window =
        SDL_CreateWindow("Test window", 100, 100, width, height, 0);
    if (window == NULL) {
        printf("SDL create window failed\n");
        return -1;
    }

    SDL_Surface *surface = SDL_GetWindowSurface(window);
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

int mmap_file(const char *filename, uint8_t **data) {
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

    void *mmap_ret = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, 0);
    if (mmap_ret == (void *) -1) {
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
