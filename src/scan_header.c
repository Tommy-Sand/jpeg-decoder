#include "scan_header.h"

#include <stdio.h>
#include <stdlib.h>

#include "debug.h"

// See B.2.3 Scan header syntax in JPEG spec
// For clarification on decoding process
int32_t decode_scan_header(Bitstream *bs, ScanHeader *sh) {
    uint16_t len = next_byte(bs) << 8;
    len += next_byte(bs);
    const uint16_t min_len = 6;
    if (len < min_len) {
        return -1;
    }

    sh->nics = next_byte(bs);
    const uint8_t max_comps = 4;
    if (sh->nics > max_comps || sh->nics == 0) {
        return -1;
    }
    for (int i = 0; i < sh->nics; i++) {
        (sh->ics[i]).sc = next_byte(bs);
        (sh->ics[i]).dc = get_byte(bs) >> 4;
        (sh->ics[i]).ac = next_byte(bs) & 0xF;
    }

    sh->ss = next_byte(bs);
    sh->se = next_byte(bs);
    sh->ah = get_byte(bs) >> 4;
    sh->al = next_byte(bs) & 0xF;

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
