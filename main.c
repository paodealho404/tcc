#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "hps_0.h"
//settings for the lightweight HPS-to-FPGA bridge
#define HW_REGS_BASE ( ALT_STM_OFST )
#define HW_REGS_SPAN ( 0x04000000 ) //64 MB with 32 bit adress space this is 256 MB
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )
 
 
//setting for the HPS2FPGA AXI Bridge
#define ALT_AXI_FPGASLVS_OFST (0xC0000000) // axi_master
#define HW_FPGA_AXI_SPAN (0x40000000) // Bridge span 1GB
#define HW_FPGA_AXI_MASK ( HW_FPGA_AXI_SPAN - 1 )

static const struct spi {
    void *sck_addr;
    void *ss_addr;
    void *mosi_addr;
    void *miso_addr;
} spi_t;

int setup_mem_addr(){
    
	void *virtual_base;
	int fd;

	// map the address space for the LED registers into user space so we can interact with them.
	// we'll actually map in the entire CSR span of the HPS since we want to access various registers within that span

	if( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) {
		printf( "ERROR: could not open \"/dev/mem\"...\n" );
		return( 1 );
	}

	virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, HW_REGS_BASE );

	if( virtual_base == MAP_FAILED ) {
		printf( "ERROR: mmap() failed...\n" );
		close( fd );
		return( 1 );
	}
	
	h2p_lw_led_addr=virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + PIO_LED_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
	

    return 0;
}

int main() {
    printf("Initializing data transfer!\n");
    uint32_t *sck_addr = 

    while (1) {
        IOWR_ALTERA_AVALON_PIO_DATA(SPI_SS_BASE, 0);
        IOWR_ALTERA_AVALON_PIO_DATA(SPI_SCK_BASE, 0);
        IOWR_ALTERA_AVALON_PIO_DATA(SPI_SS_BASE, 1);
        IOWR_ALTERA_AVALON_PIO_DATA(SPI_SCK_BASE, 1);
    }
    return 0;
}
