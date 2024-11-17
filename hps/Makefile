TARGET = tcc 
IMG = img_r_channel.txt
IP_ADDRESS = 192.168.0.96
DEBUG=0

ALT_DEVICE_FAMILY ?= soc_cv_av
PROJECT_ROOT = C:\intelFPGA\20.1\embedded\tcc
SOCEDS_ROOT ?= $(SOCEDS_DEST_ROOT)
HWLIBS_ROOT = $(SOCEDS_ROOT)/ip/altera/hps/altera_hps/hwlib
CFLAGS = -g -Wall -D$(ALT_DEVICE_FAMILY) -I$(HWLIBS_ROOT)/include/$(ALT_DEVICE_FAMILY) -I$(HWLIBS_ROOT)/include/ -DDEBUG=$(DEBUG) -I$(PROJECT_ROOT)
LDFLAGS = -g -Wall
CC = arm-none-linux-gnueabihf-gcc
ARCH= arm
 
build: $(TARGET) 
 
$(TARGET): main.o  image.o spi.o pdi.o
	$(CC) $(LDFLAGS)   $^ -o $@  
 
%.o : %.c 
	$(CC) $(CFLAGS) -c $< -o $@ 
 
.PHONY: clean 
clean: 
	rm -f $(TARGET) *.a *.o *~

flash:
	. ./send_exe.sh $(IP_ADDRESS) $(TARGET)

read_img:
	. ./read_img.sh $(IP_ADDRESS) $(IMG)

all:
	make clean
	make build
	make flash
