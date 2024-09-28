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

// Configurações para o HPS2FPGA AXI Bridge
#define ALT_AXI_FPGASLVS_OFST (0xC0000000) // axi_master
#define HW_FPGA_AXI_SPAN      (0x40000000) // Bridge span 1GB
#define HW_FPGA_AXI_MASK      (HW_FPGA_AXI_SPAN - 1)

// Definições do SPI
#define SPI_SCK_DIV     5                                     // Divisor de clock do SPI
#define SPI_COMM_PERIOD (1.0 / (SPI_SCK_FREQ >> SPI_SCK_DIV)) // Período de comunicação SPI

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

// Função para transmitir um byte via SPI
void spi_send_byte(struct spi *spi, uint8_t byte)
{
	for (int i = 7; i >= 0; i--) { // SPI é geralmente MSB first
		uint8_t bit = (byte >> i) & 0x1;
		set_mosi(spi, bit);                // Envia o bit atual
		usleep(SPI_COMM_PERIOD * 1000000); // Espera o período do clock SPI
		toggle_sck(spi);                   // Troca o clock
		usleep(SPI_COMM_PERIOD * 1000000); // Espera o período do clock SPI
		toggle_sck(spi);                   // Troca de volta o clock
	}
}

// Função para receber um byte via SPI (leitura do pino MISO)
uint8_t spi_receive_byte(struct spi *spi)
{
	uint8_t received_byte = 0;

	for (int i = 7; i >= 0; i--) {             // SPI é geralmente MSB first
		toggle_sck(spi);                   // Troca o clock
		usleep(SPI_COMM_PERIOD * 1000000); // Espera o período do clock SPI

		// Lê o bit de MISO e insere no byte recebido
		uint8_t bit = (*(spi->miso_addr) & 0x1);
		received_byte |= (bit << i);

		toggle_sck(spi);                   // Troca o clock de volta
		usleep(SPI_COMM_PERIOD * 1000000); // Espera o período do clock SPI
	}

#if DEBUG_ENABLED
	printf("Received byte: 0x%X\n", received_byte);
#endif
	return received_byte;
}

int main()
{
	struct spi spi_fields = {0}; // Estrutura do SPI inicializada

	printf("Iniciando a transferência de dados via SPI!\n");

	int err = 0;

	// err = setup_mem_addr(&spi_fields); // Assumimos que esta função está correta
	if (err) {
		printf("Erro ao configurar os endereços de memória\n");
		return err;
	}

	printf("SPI Inicializado!\n");

	uint8_t data_to_send[] = {0xA5, 0xC3, 0x7F}; // Exemplo de array de bytes a serem enviados
	size_t data_len = sizeof(data_to_send) / sizeof(data_to_send[0]);

	for (size_t i = 0; i < data_len; i++) {
		printf("Enviando byte %zu: 0x%X\n", i, data_to_send[i]);
		clear_ss(&spi_fields);                       // Seleciona o slave
		spi_send_byte(&spi_fields, data_to_send[i]); // Envia o byte

		// Leitura do SPI após enviar um byte (opcional)
		uint8_t received = spi_receive_byte(&spi_fields);
		printf("Byte recebido: 0x%X\n", received);

		set_ss(&spi_fields); // Libera o slave após a transferência

#if DEBUG_ENABLED
		usleep(1000); // Pequeno atraso entre bytes, se DEBUG estiver habilitado
#endif
	}

	printf("Transferência de dados concluída!\n");

	return 0;
}
