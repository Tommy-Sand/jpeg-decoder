SRC_DIR = ./src/
BUILD_DIR = ./build/

OBJ_FILES = jpeg_decoder.o scan_header.o huff_table.o quant_table.o frame_header.o restart_interval.o decode.o
EXT_LIBS = -lm -lSDL2

jpeg_decoder: $(OBJ_FILES)
	gcc -o $(BUILD_DIR)jpeg_decoder $(BUILD_DIR)jpeg_decoder.o $(BUILD_DIR)scan_header.o $(BUILD_DIR)huff_table.o $(BUILD_DIR)quant_table.o $(BUILD_DIR)frame_header.o $(BUILD_DIR)restart_interval.o $(BUILD_DIR)decode.o $(EXT_LIBS)

jpeg_decoder.o:
	gcc -g -c -Wall -std=c17 -o $(BUILD_DIR)jpeg_decoder.o $(SRC_DIR)jpeg_decoder.c

scan_header.o:
	gcc -g -c -Wall -std=c17 -o $(BUILD_DIR)scan_header.o $(SRC_DIR)scan_header.c
	
huff_table.o:
	gcc -g -c -Wall -std=c17 -o $(BUILD_DIR)huff_table.o $(SRC_DIR)huff_table.c
	
quant_table.o:
	gcc -g -c -Wall -std=c17 -o $(BUILD_DIR)quant_table.o $(SRC_DIR)quant_table.c
	
frame_header.o:
	gcc -g -c -Wall -std=c17 -o $(BUILD_DIR)frame_header.o $(SRC_DIR)frame_header.c
	
restart_interval.o: 
	gcc -g -c -Wall -std=c17 -o $(BUILD_DIR)restart_interval.o $(SRC_DIR)restart_interval.c
	
decode.o: 
	gcc -g -c -Wall -std=c17 -o $(BUILD_DIR)decode.o $(SRC_DIR)decode.c

clean:
	rm $(BUILD_DIR)*
