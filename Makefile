TARGET = tcc 
IP_ADDRESS = 192.168.0.8

ALT_DEVICE_FAMILY ?= soc_cv_av
SOCEDS_ROOT ?= $(SOCEDS_DEST_ROOT)
HWLIBS_ROOT = $(SOCEDS_ROOT)/ip/altera/hps/altera_hps/hwlib
CFLAGS = -g -Wall   -D$(ALT_DEVICE_FAMILY) -I$(HWLIBS_ROOT)/include/$(ALT_DEVICE_FAMILY)   -I$(HWLIBS_ROOT)/include/	-DDEBUG=1
LDFLAGS =  -g -Wall
CC = arm-none-linux-gnueabihf-gcc
ARCH= arm
 
build: $(TARGET) 
 
$(TARGET): main.o  
	$(CC) $(LDFLAGS)   $^ -o $@  
 
%.o : %.c 
	$(CC) $(CFLAGS) -c $< -o $@ 
 
.PHONY: clean 
clean: 
	rm -f $(TARGET) *.a *.o *~

flash:
	. ./send_exe.sh $(IP_ADDRESS) $(TARGET)