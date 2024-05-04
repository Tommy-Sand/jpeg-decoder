# JPEG Decoder

This is a JPEG decoder that can decode jpeg files and display them using SDL. It is written from scratch using just C and SDL2 dependency. It has an optimized fast discrete cosine transform to allow jpegs to be decoded quickly.

# Building
In order to build the application you must have a C compiler (this project uses gcc) and SDL2-dev installed
```
git clone https://github.com/Tommy-Sand/jpeg-decoder.git
cd jpeg-decoder\
make
```
The binary will be located under `build/jpeg_decoder`
# Running 
`./jpeg_decoder <JPEG file>`

# Compliance 
This JPEG decoder is capable of decoding Baseline Sequential DCT encoded JPEGs although support for other processes is planned and partially implemented.
