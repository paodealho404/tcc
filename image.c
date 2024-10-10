#include "image.h"

uint8_t image[IMAGE_HEIGHT * IMAGE_WIDTH] = {0};
void initializeImage()
{
	srand((uint32_t)0x32854232);
	for (int i = 0; i < IMAGE_HEIGHT * IMAGE_WIDTH; ++i) {
		image[i] = rand() % 256;
	}
}