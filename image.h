#ifndef IMAGE_H
#define IMAGE_H

#include <stdlib.h>
#include <stdint.h>


#define IMAGE_HEIGHT 240
#define IMAGE_WIDTH  320

extern uint8_t image[IMAGE_HEIGHT * IMAGE_WIDTH];

void initializeImage();

#endif