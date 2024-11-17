#ifndef SPI_H
#define SPI_H

#include <stdint.h>

#include <stdio.h>
#include "hps_0.h"
#include "hwlib.h"
#include "soc_cv_av/socal/alt_gpio.h"
#include "soc_cv_av/socal/hps.h"
#include "soc_cv_av/socal/socal.h"
#include "errno.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <unistd.h>
#include <stdint.h>
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)                                                                       \
	((byte) & 0x80 ? '1' : '0'), ((byte) & 0x40 ? '1' : '0'), ((byte) & 0x20 ? '1' : '0'),     \
		((byte) & 0x10 ? '1' : '0'), ((byte) & 0x08 ? '1' : '0'),                          \
		((byte) & 0x04 ? '1' : '0'), ((byte) & 0x02 ? '1' : '0'),                          \
		((byte) & 0x01 ? '1' : '0')
#define UNUSED(x) (void)(x)

uint8_t spi_receive_byte();
void spi_send_byte(uint8_t byte);
int setup_mem_addr();
int bringup_sequence();

#endif