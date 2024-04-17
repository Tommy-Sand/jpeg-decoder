#include "scan_header.h"
#include <stdio.h>
#include <stdlib.h>

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
	if (sh->nics >= 0) {
		sh->ics = (ImageComponent *) malloc(sizeof(ImageComponent) * sh->nics);
		for(uint8_t i = 0; i < sh->nics; i++) {
			ImageComponent *ic = sh->ics + i;
			
			ic->sc = *(ptr++);
			ic->dc = *ptr >> 4;
			ic->ac = (*(ptr++)) & 0xF;
		}
	}

	sh->ss = *(ptr++);
	sh->se = *(ptr++);
	sh->ah = (*ptr) >> 4;
	sh->al = (*(ptr++)) & 0xF;

	if (0) {
		printf("nics : %d\n", sh->nics);
		for (uint8_t i = 0; i < sh->nics; i++) {
			ImageComponent *ic = sh->ics + i;
			printf("sc : %d\n", ic->sc);
			printf("dc : %d\n", ic->dc);
			printf("ac : %d\n", ic->ac);
		}
		printf("ss : %d\n", sh->ss);
		printf("se : %d\n", sh->se);
		printf("ah : %d\n", sh->ah);
		printf("al : %d\n", sh->al);
	}

	*encoded_data = ptr;
	return 0;
}

void print_scan_header(ScanHeader *sh) {
	if (sh == NULL) {
		return;
	}

	printf("Scan Header\n"
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

void print_image_component(ImageComponent *ics, int len) {
	if (!ics) {
		return;
	}

	for (int i = 0; i < len; i++) {
		ImageComponent* ic = ics + i;
		printf("    Image Component: %d\n"
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
