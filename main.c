#include "errno.h"
#include "hps_0.h"
#include "hwlib.h"
#include "soc_cv_av/socal/alt_gpio.h"
#include "soc_cv_av/socal/hps.h"
#include "soc_cv_av/socal/socal.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

// Configurações para o bridge HPS-to-FPGA
#define HW_REGS_BASE (ALT_STM_OFST)
#define HW_REGS_SPAN (0x04000000) // 64 MB com espaço de endereçamento de 32 bits
#define HW_REGS_MASK (HW_REGS_SPAN - 1)

// Definições do SPI
#define SPI_SCK_DIV     5                                   // Divisor de clock do SPI
#define SPI_COMM_PERIOD (1 / (SPI_SCK_FREQ >> SPI_SCK_DIV)) // Período de comunicação SPI

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)                                                                       \
	((byte) & 0x80 ? '1' : '0'), ((byte) & 0x40 ? '1' : '0'), ((byte) & 0x20 ? '1' : '0'),     \
		((byte) & 0x10 ? '1' : '0'), ((byte) & 0x08 ? '1' : '0'),                          \
		((byte) & 0x04 ? '1' : '0'), ((byte) & 0x02 ? '1' : '0'),                          \
		((byte) & 0x01 ? '1' : '0')

#define ONE_SECOND_US 1000000

// Diretiva de compilação para DEBUG
#ifdef DEBUG
#define DEBUG_ENABLED 1
#else
#define DEBUG_ENABLED 0
#endif

struct spi {
	uint32_t *sck_addr;
	uint32_t *ss_addr;
	uint32_t *mosi_addr;
	uint32_t *miso_addr;
};

// Funções inline para controlar os pinos do SPI
static inline void set_mosi(struct spi *spi, uint8_t bit)
{
	*(spi->mosi_addr) = bit;
#if DEBUG_ENABLED
	printf("MOSI set to: %d\n", bit);
#endif
}

static inline void toggle_sck(struct spi *spi)
{
	*(spi->sck_addr) ^= 0x1;
#if DEBUG_ENABLED
	printf("SPI clock toggled\n");
#endif
}

static inline void set_ss(struct spi *spi)
{
	*(spi->ss_addr) = 0x1;
#if DEBUG_ENABLED
	printf("Slave select activated\n");
#endif
}

static inline void clear_ss(struct spi *spi)
{
	*(spi->ss_addr) = 0x0;
#if DEBUG_ENABLED
	printf("Slave select cleared\n");
#endif
}

static inline void set_sck(struct spi *spi)
{
	*(spi->sck_addr) = 0x1;
#if DEBUG_ENABLED
	printf("SCK activated\n");
#endif
}

static inline void clear_sck(struct spi *spi)
{
	*(spi->sck_addr) = 0x1;
#if DEBUG_ENABLED
	printf("SCK cleared\n");
#endif
}

int setup_mem_addr(struct spi *spi)
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

	printf("Endereco virtual do bridge: 0x%X\n", (unsigned int)virtual_base);

	spi->miso_addr = virtual_base + ((unsigned long)(ALT_LWFPGASLVS_OFST + SPI_MISO_BASE) &
					 (unsigned long)(HW_REGS_MASK));
	spi->mosi_addr = virtual_base + ((unsigned long)(ALT_LWFPGASLVS_OFST + SPI_MOSI_BASE) &
					 (unsigned long)(HW_REGS_MASK));
	spi->sck_addr = virtual_base + ((unsigned long)(ALT_LWFPGASLVS_OFST + SPI_SCK_BASE) &
					(unsigned long)(HW_REGS_MASK));
	spi->ss_addr = virtual_base + ((unsigned long)(ALT_LWFPGASLVS_OFST + SPI_SS_BASE) &
				       (unsigned long)(HW_REGS_MASK));

	*(spi->miso_addr) = 0x0;
	*(spi->mosi_addr) = 0x0;
	*(spi->sck_addr) = 0x0;
	*(spi->ss_addr) = 0x1;

	return 0;
}

// Função para transmitir um byte via SPI
void spi_send_byte(struct spi *spi, uint8_t byte)
{
	printf("Representacao binaria: " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(byte));
	printf("\n");
	for (int i = 7; i >= 0; i--) { // SPI é geralmente MSB first
		uint8_t bit = (byte >> i) & 0x1;
		set_mosi(spi, bit);                      // Envia o bit atual
		usleep(SPI_COMM_PERIOD * ONE_SECOND_US); // Espera o período do clock SPI
		toggle_sck(spi);                         // Troca o clock
		usleep(SPI_COMM_PERIOD * ONE_SECOND_US); // Espera o período do clock SPI
		toggle_sck(spi);                         // Troca de volta o clock
	}
}

// Função para receber um byte via SPI (leitura do pino MISO)
uint8_t spi_receive_byte(struct spi *spi)
{
	uint8_t received_byte = 0;

	for (int i = 7; i >= 0; i--) {                   // SPI é geralmente MSB first
		toggle_sck(spi);                         // Troca o clock
		usleep(SPI_COMM_PERIOD * ONE_SECOND_US); // Espera o período do clock SPI

		// Lê o bit de MISO e insere no byte recebido
		uint8_t bit = (*(spi->miso_addr) & 0x1);
		received_byte |= (bit << i);

		toggle_sck(spi);                         // Troca o clock de volta
		usleep(SPI_COMM_PERIOD * ONE_SECOND_US); // Espera o período do clock SPI
	}

#if DEBUG_ENABLED
	printf("Received byte: 0x%X\n", received_byte);
#endif
	return received_byte;
}

void bringup_sequence(struct spi *spi, uint8_t multiplier)
{
	// Testa a sequência de inicialização do SPI

	// Inicializa o slave select
	set_ss(spi); // Libera o slave após a transferência
	usleep(multiplier *
	       ONE_SECOND_US); // Pequeno atraso entre bytes, se DEBUG estiver habilitado
	clear_ss(spi);         // Seleciona o slave
	usleep(multiplier *
	       ONE_SECOND_US); // Pequeno atraso entre bytes, se DEBUG estiver habilitado
	set_ss(spi);           // Libera o slave após a transferência

	// Inicializa o clock SPI
	set_sck(spi); // Ativa o clock SPI
	usleep(multiplier *
	       ONE_SECOND_US); // Pequeno atraso entre bytes, se DEBUG estiver habilitado
	clear_sck(spi);        // Desativa o clock SPI
	usleep(multiplier *
	       ONE_SECOND_US); // Pequeno atraso entre bytes, se DEBUG estiver habilitado

	// Inicializa o MOSI
	set_mosi(spi, 0x1); // Ativa o MOSI
	usleep(multiplier *
	       ONE_SECOND_US); // Pequeno atraso entre bytes, se DEBUG estiver habilitado
	set_mosi(spi, 0x0);    // Desativa o MOSI
	usleep(multiplier *
	       ONE_SECOND_US); // Pequeno atraso entre bytes, se DEBUG estiver habilitado
}

int main()
{
	struct spi spi_fields; // Estrutura do SPI inicializada

	printf("Iniciando a transferencia de dados via SPI!\n");

	int err = 0;

	err = setup_mem_addr(&spi_fields); // Assumimos que esta função está correta
	if (err) {
		printf("Erro ao configurar os enderecos de memoria\n");
		return err;
	}

#if DEBUG_ENABLED
	printf("DEBUG habilitado!\n");
	bringup_sequence(&spi_fields, 5);
#endif

	uint8_t data_to_send[] = {0x04}; // Exemplo de array de bytes a serem enviados
	size_t data_len = sizeof(data_to_send) / sizeof(data_to_send[0]);

	for (size_t i = 0; i < data_len; i++) {
		printf("Enviando byte %zu: 0x%X\n", i, data_to_send[i]);
		clear_ss(&spi_fields);                       // Seleciona o slave
		spi_send_byte(&spi_fields, data_to_send[i]); // Envia o byte
		usleep(ONE_SECOND_US); // Pequeno atraso entre bytes, se DEBUG estiver habilitado
		set_ss(&spi_fields);   // Libera o slave após a transferência
		usleep(ONE_SECOND_US); // Pequeno atraso entre bytes, se DEBUG estiver habilitado
	}

	printf("Transferencia de dados concluida!\n");

	return 0;
}
