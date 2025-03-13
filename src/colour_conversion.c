#include "colour_conversion.h"

int y_rgb(
    FrameHeader *fh,
    Image *img,
    uint8_t *pixels,
    uint16_t width,
    uint16_t height,
    uint16_t pitch
) {
    uint8_t max_hsf, max_vsf;
    if (get_max_sf(fh, &max_hsf, &max_vsf) == -1) {
        return -1;
    };

    for (uint32_t i = 0; i < height; i++) {
        for (uint32_t j = 0; j < width; j++) {
            uint32_t src_idx = i * width + j;
            uint32_t dst_idx = i * pitch + j * 3;
            pixels[dst_idx] = *((*img->buf) + src_idx);
            pixels[dst_idx + 1] = *((*img->buf) + src_idx);
            pixels[dst_idx + 2] = *((*img->buf) + src_idx);
        }
    }
    return 0;
}

int ycbcr_rgb(
    FrameHeader *fh,
    Image *img,
    uint8_t *pixels,
    uint16_t width,
    uint16_t height,
    uint16_t pitch
) {
    uint8_t max_hsf, max_vsf;
    if (get_max_sf(fh, &max_hsf, &max_vsf) == -1) {
        return -1;
    };

    Component c = *fh->cs;
    float hratio0 = (float) c.hsf / max_hsf;
    float vratio0 = (float) c.vsf / max_vsf;
    uint16_t x_to_mcu0 =
        c.x + ((c.x % (8 * c.hsf)) ? (8 * c.hsf - (c.x % (8 * c.hsf))) : 0);

    c = *(fh->cs + 1);
    float hratio1 = (float) c.hsf / max_hsf;
    float vratio1 = (float) c.vsf / max_vsf;
    uint16_t x_to_mcu1 =
        c.x + ((c.x % (8 * c.hsf)) ? (8 * c.hsf - (c.x % (8 * c.hsf))) : 0);

    c = *(fh->cs + 2);
    float hratio2 = (float) c.hsf / max_hsf;
    float vratio2 = (float) c.vsf / max_vsf;
    uint16_t x_to_mcu2 =
        c.x + ((c.x % (8 * c.hsf)) ? (8 * c.hsf - (c.x % (8 * c.hsf))) : 0);

    for (uint32_t i = 0; i < height; i++) {
        for (uint32_t j = 0; j < width; j++) {
            uint8_t Y =
                *(*(img->buf)
                  + (((uint32_t) (i * vratio0) * x_to_mcu0)
                     + (uint32_t) (hratio0 * j)));
            uint8_t Cb =
                *(*(img->buf + 1)
                  + (((uint32_t) (i * vratio1) * x_to_mcu1)
                     + (uint32_t) (hratio1 * j)));
            uint8_t Cr =
                *(*(img->buf + 2)
                  + (((uint32_t) (i * vratio2) * x_to_mcu2)
                     + (uint32_t) (hratio2 * j)));

            float R = Y + 1.402 * (((float) Cr) - 128.0);
            float G = Y - 0.34414 * (((float) Cb) - 128.0)
                - 0.71414 * (((float) Cr) - 128.0);
            float B = Y + 1.772 * (((float) Cb) - 128.0);

            uint32_t idx = i * pitch + j * 3;
            pixels[idx] = (R < 0.0) ? 0 : ((R > 256.0) ? 256 : R);
            pixels[idx + 1] = (G < 0.0) ? 0 : ((G > 256.0) ? 256 : G);
            pixels[idx + 2] = (B < 0.0) ? 0 : ((B > 256.0) ? 256 : B);
        }
    }
    return 0;
}

int yccb_rgb(
    FrameHeader *fh,
    Image *img,
    uint8_t *pixels,
    uint16_t width,
    uint16_t height,
    uint16_t pitch
) {
    uint8_t max_hsf, max_vsf;
    if (get_max_sf(fh, &max_hsf, &max_vsf) == -1) {
        return -1;
    };

    Component c = *fh->cs;
    float hratio0 = (float) c.hsf / max_hsf;
    float vratio0 = (float) c.vsf / max_vsf;
    uint16_t x_to_mcu0 =
        c.x + ((c.x % (8 * c.hsf)) ? (8 * c.hsf - (c.x % (8 * c.hsf))) : 0);

    c = *(fh->cs + 1);
    float hratio1 = (float) c.hsf / max_hsf;
    float vratio1 = (float) c.vsf / max_vsf;
    uint16_t x_to_mcu1 =
        c.x + ((c.x % (8 * c.hsf)) ? (8 * c.hsf - (c.x % (8 * c.hsf))) : 0);

    c = *(fh->cs + 2);
    float hratio2 = (float) c.hsf / max_hsf;
    float vratio2 = (float) c.vsf / max_vsf;
    uint16_t x_to_mcu2 =
        c.x + ((c.x % (8 * c.hsf)) ? (8 * c.hsf - (c.x % (8 * c.hsf))) : 0);

    c = *(fh->cs + 3);
    float hratio3 = (float) c.hsf / max_hsf;
    float vratio3 = (float) c.vsf / max_vsf;
    uint16_t x_to_mcu3 =
        c.x + ((c.x % (8 * c.hsf)) ? (8 * c.hsf - (c.x % (8 * c.hsf))) : 0);

    for (uint32_t j = 0; j < height; j++) {
        for (uint32_t k = 0; k < width; k++) {
            uint8_t Y_ =
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

            float C = (Y_ + 1.402 * (((float) Cr) - 128.0));
            float M =
                (Y_ - 0.34414 * (((float) Cb) - 128.0)
                 - 0.71414 * (((float) Cr) - 128.0));
            float Y = (Y_ + 1.772 * (((float) Cb) - 128.0));

            uint8_t K =
                *(*(img->buf + 3)
                  + (((uint32_t) (j * vratio3) * x_to_mcu3)
                     + (uint32_t) (hratio3 * k)));

            float R =
                255.0 * (1.0 - ((float) C / 255.0)) * (((float) K / 255.0));
            float G =
                255.0 * (1.0 - ((float) M / 255.0)) * (((float) K / 255.0));
            float B =
                255.0 * (1.0 - ((float) Y / 255.0)) * (((float) K / 255.0));

            uint32_t idx = (j * pitch) + (k * 3);
            pixels[idx] = (R < 0.0) ? 0 : ((R > 256.0) ? 256 : R);
            pixels[idx + 1] = (G < 0.0) ? 0 : ((G > 256.0) ? 256 : G);
            pixels[idx + 2] = (B < 0.0) ? 0 : ((B > 256.0) ? 256 : B);
        }
    }
    return 0;
}

int get_max_sf(FrameHeader *fh, uint8_t *max_hsf, uint8_t *max_vsf) {
    if (fh->ncs == 0) {
        return -1;
    }
    *max_hsf = *max_vsf = 0;

    for (uint8_t i = 0; i < fh->ncs; i++) {
        Component c = *(fh->cs + i);
        if (c.hsf > *max_hsf) {
            *max_hsf = c.hsf;
        }
        if (c.vsf > *max_vsf) {
            *max_vsf = c.vsf;
        }
    }
    return 0;
}
