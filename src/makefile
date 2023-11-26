jpeg_decoder: jpeg_decoder.o scan_header.o huff_table.o quant_table.o frame_header.o restart_interval.o decode.o
	gcc -o jpeg_decoder jpeg_decoder.o scan_header.o huff_table.o quant_table.o frame_header.o restart_interval.o decode.o -lm -lSDL2

jpeg_decoder.o:
	gcc -g -c -Wall -std=c99 jpeg_decoder.c

scan_header.o:
	gcc -g -c -Wall -std=c99 scan_header.c
	
huff_table.o:
	gcc -g -c -Wall -std=c99 huff_table.c
	
quant_table.o:
	gcc -g -c -Wall -std=c99 quant_table.c
	
frame_header.o:
	gcc -g -c -Wall -std=c99 frame_header.c
	
restart_interval.o: 
	gcc -g -c -Wall -std=c99 restart_interval.c
	
decode.o: 
	gcc -g -c -Wall -std=c99 decode.c

clean:
	rm jpeg_decoder.o scan_header.o huff_table.o quant_table.o frame_header.o restart_interval.o decode.o jpeg_decoder
