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
#include <time.h>

#include "colour_conversion.h"
#include "decode.h"
#include "frame_header.h"
#include "huff_table.h"
#include "quant_table.h"
#include "restart_interval.h"
#include "scan_header.h"
#include "debug.h"

int display_image(int width, int height, SDL_Surface *image);
int read_app_segment(uint8_t **encoded_data);
int64_t mmap_file(const char *filename, uint8_t **data);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Need file path\n");
        return -1;
    }

    uint8_t *buf;
    int64_t size = mmap_file(argv[1], &buf);
    if (size == -1) {
        return -1;
    }
    debug_print("length: %lu\n", size);

    Image *img = NULL;
    FrameHeader fh;
    if (decode_jpeg_buffer(buf, size, &fh, &img) != 0) {
        fprintf(stderr, "DEBUG: Decoding jpeg buffer failed\n");
        return -1;
    };

    uint16_t width = fh.X;
    uint16_t height = fh.Y;

    if (SDL_Init(SDL_INIT_TIMER)) {
        fprintf(stderr, "SDL_Init failed\n");
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
        fprintf(stderr, "SDL_CreateRGBSurfaceWithFormat failed\n");
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
        y_rgb(&fh, img, pixels, width, height, pitch);
    } else if (fh.ncs == 3) {
        ycbcr_rgb(&fh, img, pixels, width, height, pitch);
    } else if (fh.ncs == 4) {
        yccb_rgb(&fh, img, pixels, width, height, pitch);
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
        fprintf(stderr, "SDL_CreateWindow failed\n");
        return -1;
    }

    SDL_Surface *surface = SDL_GetWindowSurface(window);
    if (SDL_BlitSurface(image, NULL, surface, NULL)) {
        fprintf(stderr, "SDL_BlitSurface failed\n");
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
		struct timespec request = { 0, 60 * 1000 * 1000 };
		nanosleep(&request, NULL); 
    }
    return 0;
}

int64_t mmap_file(const char *filename, uint8_t **data) {
    struct stat st;
    if (stat(filename, &st) == -1) {
        fprintf(stderr, "Could not get size of file, %s\n", strerror(errno));
        return -1;
    }
    off_t len = st.st_size;

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Could not open file, %s\n", strerror(errno));
        return -1;
    }

    void *mmap_ret = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, 0);
    if (mmap_ret == (void *) -1) {
        fprintf(stderr, "Could not get contents of file, %s\n", strerror(errno));
        return -1;
    }
    *data = mmap_ret;

    if (close(fd) == -1) {
        fprintf(stderr, "Could not close file descriptor, %s\n", strerror(errno));
        return -1;
    };
    return len;
}
