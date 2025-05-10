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

#include "colour_conversion.h"
#include "decode.h"
#include "frame_header.h"
#include "huff_table.h"
#include "quant_table.h"
#include "restart_interval.h"
#include "scan_header.h"

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
    printf("length: %lu\n", size);

    Image *img = NULL;
    FrameHeader fh;
    if (encode_jpeg_buffer(buf, size, &fh, &img) != 0) {
        printf("DEBUG: Decoding jpeg buffer failed\n");
        return -1;
    };

    uint16_t width = fh.X;
    uint16_t height = fh.Y;

    uint16_t pitch = img_surface->pitch;
    width = img_surface->w;
    height = img_surface->h;
    uint8_t *pixels = img_surface->pixels;

    munmap(buf, size);
    free_frame_header(&fh);
    return 0;
}
