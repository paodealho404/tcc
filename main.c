#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "hps_0.h"
#include "hwlib.h"
#include "soc_cv_av/socal/socal.h"
#include "soc_cv_av/socal/hps.h"
#include "soc_cv_av/socal/alt_gpio.h"
#include "errno.h"
#include <unistd.h>

// settings for the lightweight HPS-to-FPGA bridge
#define HW_REGS_BASE (ALT_STM_OFST)
#define HW_REGS_SPAN (0x04000000) // 64 MB with 32 bit adress space this is 256 MB
#define HW_REGS_MASK (HW_REGS_SPAN - 1)

// setting for the HPS2FPGA AXI Bridge
#define ALT_AXI_FPGASLVS_OFST (0xC0000000) // axi_master
#define HW_FPGA_AXI_SPAN (0x40000000)	   // Bridge span 1GB
#define HW_FPGA_AXI_MASK (HW_FPGA_AXI_SPAN - 1)

#define SPI_SCK_DIV 8
#define SPI_COMM_PERIOD 1/(SPI_SCK_FREQ>>SPI_SCK_DIV)

static struct spi
{
	uint32_t *sck_addr;
	uint32_t *ss_addr;
	uint32_t *mosi_addr;
	uint32_t *miso_addr;
} spi_fields = {
	.sck_addr = NULL,
	.ss_addr = NULL,
	.mosi_addr = NULL,
	.miso_addr = NULL};

int setup_mem_addr(struct spi *spi)
{

	void *virtual_base;
	void *axi_virtual_base;
	int fd;

	// map the address space for the LED registers into user space so we can interact with them.
	// we'll actually map in the entire CSR span of the HPS since we want to access various registers within that span

	if ((fd = open("/dev/mem", (O_RDWR | O_SYNC))) == -1)
	{
		printf("ERROR: could not open \"/dev/mem\"...\n");
		return -EIO;
	}

	virtual_base = mmap(NULL, HW_REGS_SPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, HW_REGS_BASE);

	if (virtual_base == MAP_FAILED)
	{
		printf("ERROR: mmap() failed...\n");
		close(fd);
		return -EFAULT;
	}

	axi_virtual_base = mmap(NULL, HW_FPGA_AXI_SPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, ALT_AXI_FPGASLVS_OFST);

	if (axi_virtual_base == MAP_FAILED)
	{
		printf("ERROR: axi mmap() failed...\n");
		close(fd);
		return -EFAULT;
	}

	spi->miso_addr = axi_virtual_base + ((unsigned long)(ALT_LWFPGASLVS_OFST + SPI_MISO_BASE) & (unsigned long)(HW_REGS_MASK));
	spi->mosi_addr = axi_virtual_base + ((unsigned long)(ALT_LWFPGASLVS_OFST + SPI_MOSI_BASE) & (unsigned long)(HW_REGS_MASK));
	spi->sck_addr = axi_virtual_base + ((unsigned long)(ALT_LWFPGASLVS_OFST + SPI_SCK_BASE) & (unsigned long)(HW_REGS_MASK));
	spi->ss_addr = axi_virtual_base + ((unsigned long)(ALT_LWFPGASLVS_OFST + SPI_SS_BASE) & (unsigned long)(HW_REGS_MASK));

	*(spi->miso_addr) = 0x0;
	*(spi->mosi_addr) = 0x0;
	*(spi->sck_addr) = 0x0;
	*(spi->ss_addr) = 0x1;
	
	return 0;
}

// void send_header();
// enum colors {
// 	C_RED = 0,

// } color;

// void serialize_channel(enum colors);

// void result();
static inline void set_mosi(struct spi *spi)
{
	*(spi->mosi_addr) = 0x1;
}

static inline void clear_mosi(struct spi *spi)
{
	*(spi->mosi_addr) = 0x0;
}

static inline void toggle_sck(struct spi *spi)
{
	*(spi->sck_addr) ^= 0x1;
}

static inline void set_ss(struct spi *spi)
{
	*(spi->ss_addr) = 0x1;
}

static inline void clear_ss(struct spi *spi)
{
	*(spi->ss_addr) = 0x0;
}

int main()
{
	printf("Initializing data transfer!\n");
	int err = 0;

	err = setup_mem_addr(&spi_fields);
	if (err)
	{
		printf("Error setting up memory addresses\n");
		return err;
	}

	printf("SPI Initialized!\n");
	uint8_t count = 0;
	while (1)
	{
		printf("Looping...\n");

		if(count == 0xFF){
			break;
		}

		set_ss(&spi_fields);

		toggle_sck(&spi_fields);

		if (*(spi_fields.sck_addr) == 0) {
			continue;
		}
		
		set_mosi(&spi_fields);

		usleep(SPI_COMM_PERIOD * 1000000 * 10);
		count +=1;
		
	}
	return 0;
}
