SRC_DIR = ./src/
BUILD_DIR = ./build/

CC = gcc

FLAGS = -Wall -std=c17 -Wextra

OBJ_FILES = jpeg_decoder.o scan_header.o huff_table.o quant_table.o frame_header.o restart_interval.o decode.o colour_conversion.o
EXT_LIBS = -lm -lSDL2

jpeg_decoder: $(OBJ_FILES)
	$(CC) -o $(BUILD_DIR)jpeg_decoder $(BUILD_DIR)jpeg_decoder.o $(BUILD_DIR)scan_header.o $(BUILD_DIR)huff_table.o $(BUILD_DIR)quant_table.o $(BUILD_DIR)frame_header.o $(BUILD_DIR)restart_interval.o $(BUILD_DIR)decode.o $(BUILD_DIR)colour_conversion.o $(EXT_LIBS)

jpeg_decoder.o:
	$(CC) -g -c $(FLAGS) -o $(BUILD_DIR)jpeg_decoder.o $(SRC_DIR)jpeg_decoder.c

scan_header.o:
	$(CC) -g -c $(FLAGS) -o $(BUILD_DIR)scan_header.o $(SRC_DIR)scan_header.c
	
huff_table.o:
	$(CC) -g -c $(FLAGS) -o $(BUILD_DIR)huff_table.o $(SRC_DIR)huff_table.c
	
quant_table.o:
	$(CC) -g -c $(FLAGS) -o $(BUILD_DIR)quant_table.o $(SRC_DIR)quant_table.c
	
frame_header.o:
	$(CC) -g -c $(FLAGS) -o $(BUILD_DIR)frame_header.o $(SRC_DIR)frame_header.c
	
restart_interval.o: 
	$(CC) -g -c $(FLAGS) -o $(BUILD_DIR)restart_interval.o $(SRC_DIR)restart_interval.c
	
decode.o: 
	$(CC) -g -c $(FLAGS) -o $(BUILD_DIR)decode.o $(SRC_DIR)decode.c

dct.o:
	$(CC) -g -c $(FLAGS) -o $(BUILD_DIR)dct.o $(SRC_DIR)dct.c

colour_conversion.o:
	$(CC) -g -c $(FLAGS) -o $(BUILD_DIR)colour_conversion.o $(SRC_DIR)colour_conversion.c

clean:
	rm $(BUILD_DIR)*
