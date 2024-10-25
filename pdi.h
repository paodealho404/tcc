#ifndef PDI_H
#define PDI_H

#include "spi.h"
#include <time.h>

#define NO_RETURN_MASK   0b00000000
#define PDI_RUNNING_MASK 0b01000000

#define NO_OP_MASK         0b00000000
#define SEND_IMAGE_OP_MASK 0b00000100
#define RECV_IMAGE_OP_MASK 0b00001000
#define PDI_EXEC_OP_MASK   0b00001100
#define GESTURE_EVAL_MASK  0b00011100
#define HAND_AREA_MASK     0b00010000
#define HAND_PER_MASK      0b00010100
#define HAND_PEAK_MASK     0b00011000

#define IMAGE_CHN_DFT 0b00000000
#define IMAGE_CHN_R   0b00000001
#define IMAGE_CHN_G   0b00000010
#define IMAGE_CHN_B   0b00000011

#define IMG_HEIGHT 240
#define IMG_WIDTH  320

int execute_pdi();

#endif