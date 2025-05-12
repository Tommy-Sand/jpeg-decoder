#include "scan_header.h"

#include <stdio.h>
#include <stdlib.h>

#include "debug.h"

int32_t decode_scan_header(uint8_t **encoded_data, ScanHeader *sh) {
    if (encoded_data == NULL || *encoded_data == NULL || sh == NULL) {
        return -1;
    }
    uint8_t *ptr = *encoded_data;

    uint16_t len = (*(ptr++)) << 8;
    len += *(ptr++);
    if (len < 6) {
        return -1;
    }

    sh->nics = *(ptr++);
    if (sh->nics > 4 || sh->nics == 0) {
        return -1;
    }
    for (uint8_t i = 0; i < sh->nics; i++) {
        (sh->ics[i]).sc = *(ptr++);
        (sh->ics[i]).dc = *ptr >> 4;
        (sh->ics[i]).ac = (*(ptr++)) & 0xF;
    }

    sh->ss = *(ptr++);
    sh->se = *(ptr++);
    sh->ah = (*ptr) >> 4;
    sh->al = (*(ptr++)) & 0xF;

    *encoded_data = ptr;
    return 0;
}

void print_scan_header(ScanHeader *sh) {
    if (sh == NULL) {
        return;
    }

    if (DEBUG) {
        fprintf(
            stderr,
            "Scan Header\n"
            "Start of Spectral Selection: %d\n"
            "End of Spectral Selection: %d\n"
            "Successive Approximation pos high: %d\n"
            "Successive Approximation pos low: %d\n",
            sh->ss,
            sh->se,
            sh->ah,
            sh->al
        );

        print_image_component(sh->ics, sh->nics);
    }
}

void print_image_component(ImageComponent *ics, int len) {
    if (!ics) {
        return;
    }

    if (DEBUG) {
        for (int i = 0; i < len; i++) {
            ImageComponent *ic = ics + i;
            fprintf(
                stderr,
                "    Image Component: %d\n"
                "        Scan Selector Component: %d\n"
                "        DC entropy table: %d\n"
                "        AC entropy table: %d\n",
                i,
                ic->sc,
                ic->dc,
                ic->ac
            );
        }
    }
}
