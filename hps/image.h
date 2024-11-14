#ifndef IMAGE_H
#define IMAGE_H

#include <stdlib.h>
#include <stdint.h>

#define IMAGE_HEIGHT 240
#define IMAGE_WIDTH  320

extern const uint8_t img_r_channel[IMAGE_HEIGHT * IMAGE_WIDTH];
extern const uint8_t img_g_channel[IMAGE_HEIGHT * IMAGE_WIDTH];
extern const uint8_t img_b_channel[IMAGE_HEIGHT * IMAGE_WIDTH];

#endif