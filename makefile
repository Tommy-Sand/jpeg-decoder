SRC_DIR = ./src/
BUILD_DIR = ./build/

CC = gcc
FLAGS = -g -c -Wall -std=c17 -Wextra -O2

ASM = nasm
ASM_FLAGS = -felf64

OBJ_FILES = jpeg_decoder.o scan_header.o huff_table.o quant_table.o frame_header.o restart_interval.o dct.o decode.o colour_conversion.o encode.o bitstream.o maxofthree.o
EXT_LIBS = -lm -lSDL2

jpeg_decoder: $(OBJ_FILES)
	$(CC) -o $(BUILD_DIR)jpeg_decoder $(BUILD_DIR)jpeg_decoder.o $(BUILD_DIR)scan_header.o $(BUILD_DIR)huff_table.o $(BUILD_DIR)quant_table.o $(BUILD_DIR)frame_header.o $(BUILD_DIR)restart_interval.o $(BUILD_DIR)dct.o $(BUILD_DIR)decode.o $(BUILD_DIR)colour_conversion.o $(BUILD_DIR)encode.o $(BUILD_DIR)bitstream.o $(BUILD_DIR)maxofthree.o $(EXT_LIBS) -no-pie 

jpeg_decoder.o:
	$(CC) $(FLAGS) -o $(BUILD_DIR)jpeg_decoder.o $(SRC_DIR)jpeg_decoder.c

scan_header.o:
	$(CC) $(FLAGS) -o $(BUILD_DIR)scan_header.o $(SRC_DIR)scan_header.c
	
huff_table.o:
	$(CC) $(FLAGS) -o $(BUILD_DIR)huff_table.o $(SRC_DIR)huff_table.c
	
quant_table.o: dequant_quant_table.o
	$(CC) $(FLAGS) -o $(BUILD_DIR)quant_table.o $(SRC_DIR)quant_table.c -no-pie
	
dequant_quant_table.o:
	nasm -g -felf64 -o $(BUILD_DIR)dequant_quant_table.o $(SRC_DIR)dequant_quant_table.asm

frame_header.o:
	$(CC) $(FLAGS) -o $(BUILD_DIR)frame_header.o $(SRC_DIR)frame_header.c
	
restart_interval.o: 
	$(CC) $(FLAGS) -o $(BUILD_DIR)restart_interval.o $(SRC_DIR)restart_interval.c
	
decode.o: 
	$(CC) $(FLAGS) -o $(BUILD_DIR)decode.o $(SRC_DIR)decode.c

encode.o: 
	$(CC) $(FLAGS) -o $(BUILD_DIR)encode.o $(SRC_DIR)encode.c

dct.o:
	$(CC) $(FLAGS) -o $(BUILD_DIR)dct.o $(SRC_DIR)dct.c

colour_conversion.o:
	$(CC) $(FLAGS) -o $(BUILD_DIR)colour_conversion.o $(SRC_DIR)colour_conversion.c

bitstream.o:
	$(CC) $(FLAGS) -o $(BUILD_DIR)bitstream.o $(SRC_DIR)bitstream.c

maxofthree.o:
	$(ASM) $(ASM_FLAGS) -o $(BUILD_DIR)maxofthree.o $(SRC_DIR)maxofthree.asm
clean:
	rm $(BUILD_DIR)*
