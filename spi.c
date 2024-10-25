#include "spi.h"
/* Protocolo:
 * Byte 1 -> Comando -> Retorno FPGA [7-6] | Operação [5-2] | Canal da imagem [1-0]
 * Bytes 2-3 -> Altura da imagem
 * Bytes 3-4 -> Largura da imagem
 * Bytes restantes -> Pixels da imagem
 *
 * Retorno FPGA: 00 -> Sem retorno | 01 -> PDI em execução,
 *
 * Operação: 0000 -> Nenhuma operação | 0001 -> Envio de imagem | 0010 -> Recebimento de
 * imagem | 0011 -> Execução de PDI | 0111 -> Classificação do gesto,
 *
 * Canal da imagem: 00 -> Canal padrão (R) | 01 -> Canal 1 (R) | 02 -> Canal 2(G) |
 * 11 -> Canal 3 (B).
 */

#define DELAY_MAGIC_NUMBER 1
// Configurações para o bridge HPS-to-FPGA
#define HW_REGS_BASE       (ALT_STM_OFST)
#define HW_REGS_SPAN       (0x04000000) // 64 MB com espaço de endereçamento de 32 bits
#define HW_REGS_MASK       (HW_REGS_SPAN - 1)

#define ONE_SECOND_NS (1000000000)

// Diretiva de compilação para DEBUG

static struct spi {
	uint32_t *sck_addr;
	uint32_t *ss_addr;
	uint32_t *mosi_addr;
	uint32_t *miso_addr;
} spi_fields = {0};
// Funções inline para controlar os pinos do SPI
static inline void set_mosi(uint8_t bit)
{
	*(spi_fields.mosi_addr) = bit;
}

static inline void toggle_sck()
{
	*(spi_fields.sck_addr) ^= 0x1;
}

static inline void set_ss()
{
	*(spi_fields.ss_addr) = 0x1;
}

static inline void clear_ss()
{
	*(spi_fields.ss_addr) = 0x0;
}

static inline void set_sck()
{
	*(spi_fields.sck_addr) = 0x1;
}

static inline void clear_sck()
{
	*(spi_fields.sck_addr) = 0x0;
}

int bringup_sequence()
{
	// Testa a sequência de inicialização do SPI

	set_sck(); // Ativa o clock SPI

	set_mosi(0x1); // Ativa o MOSI

	clear_sck(); // Desativa o clock SPI

	set_mosi(0x0); // Desativa o MOSI

	clear_ss(); // Seleciona o slave

	// Inicializa o slave select
	set_ss(); // Libera o slave após a transferência

	return 0;
}

static inline void spi_change_to_default()
{
	*(spi_fields.mosi_addr) = 0x0;
	*(spi_fields.sck_addr) = 0x0;
	*(spi_fields.ss_addr) = 0x1;
}

int setup_mem_addr()
{

	void *virtual_base;
	int fd;

	// map the address space for the LED registers into user space so we can interact with them.
	// we'll actually map in the entire CSR span of the HPS since we want to access various
	// registers within that span

	if ((fd = open("/dev/mem", (O_RDWR | O_SYNC))) == -1) {
		printf("ERROR: could not open \"/dev/mem\"...\n");
		return -EIO;
	}

	virtual_base =
		mmap(NULL, HW_REGS_SPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, HW_REGS_BASE);

	if (virtual_base == MAP_FAILED) {
		printf("ERROR: mmap() failed...\n");
		close(fd);
		return -EFAULT;
	}

#if DEBUG == 1
	printf("Endereco virtual do bridge: 0x%X\n", (unsigned int)virtual_base);
#endif

	spi_fields.miso_addr =
		virtual_base + ((unsigned long)(ALT_LWFPGASLVS_OFST + SPI_MISO_BASE) &
				(unsigned long)(HW_REGS_MASK));
	spi_fields.mosi_addr =
		virtual_base + ((unsigned long)(ALT_LWFPGASLVS_OFST + SPI_MOSI_BASE) &
				(unsigned long)(HW_REGS_MASK));
	spi_fields.sck_addr = virtual_base + ((unsigned long)(ALT_LWFPGASLVS_OFST + SPI_SCK_BASE) &
					      (unsigned long)(HW_REGS_MASK));
	spi_fields.ss_addr = virtual_base + ((unsigned long)(ALT_LWFPGASLVS_OFST + SPI_SS_BASE) &
					     (unsigned long)(HW_REGS_MASK));

	spi_change_to_default();
	return 0;
}
static inline void delay(uint32_t cyc)
{
	uint32_t cycles = cyc;
	while (cycles--) {
	}
}
// Função para transmitir um byte via SPI
void spi_send_byte(uint8_t byte)
{
	clear_ss();                        // Seleciona o slave
	for (int i = 7; i >= 0; i--) {     // SPI é geralmente MSB first
		clear_sck();               // Troca o clock
		delay(DELAY_MAGIC_NUMBER); // Troca o clock

		set_mosi((byte >> i) & 0x1); // Envia o bit atual
		set_sck();                   // Troca de volta o clock
		delay(DELAY_MAGIC_NUMBER);
	}
	spi_change_to_default(); // Volta para o estado inicial
}

// Função para receber um byte via SPI (leitura do pino MISO)
uint8_t spi_receive_byte()
{
	uint8_t received_byte = 0;

	clear_ss();                    // Seleciona o slave
	for (int i = 7; i >= 0; i--) { //  é geralmente MSB first
		clear_sck();           // Troca o clock
		delay(DELAY_MAGIC_NUMBER);

		set_sck(); // Troca o clock de volta
		uint8_t bit = (*(spi_fields.miso_addr) & 0x1);
		delay(DELAY_MAGIC_NUMBER);

		received_byte |= (bit << i);
	}

	spi_change_to_default(); // Volta para o estado inicial
	return received_byte;
}