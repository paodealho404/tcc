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
#include <stdint.h>

// Configurações para o bridge HPS-to-FPGA
#define HW_REGS_BASE (ALT_STM_OFST)
#define HW_REGS_SPAN (0x04000000) // 64 MB com espaço de endereçamento de 32 bits
#define HW_REGS_MASK (HW_REGS_SPAN - 1)

// Definições do SPI
#define SPI_SCK_DIV     4                                       // Divisor de clock do SPI
#define SPI_COMM_PERIOD (1 / 2 * (SPI_SCK_FREQ >> SPI_SCK_DIV)) // Período de comunicação SPI

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)                                                                       \
	((byte) & 0x80 ? '1' : '0'), ((byte) & 0x40 ? '1' : '0'), ((byte) & 0x20 ? '1' : '0'),     \
		((byte) & 0x10 ? '1' : '0'), ((byte) & 0x08 ? '1' : '0'),                          \
		((byte) & 0x04 ? '1' : '0'), ((byte) & 0x02 ? '1' : '0'),                          \
		((byte) & 0x01 ? '1' : '0')

#define GET_MSB_16BIT(var) ((uint8_t)((var) >> 8))
#define GET_LSB_16BIT(var) ((uint8_t)((var) & 0xFF))
#define UNUSED(x)          (void)(x)
#define ONE_SECOND_US      1000000

// Diretiva de compilação para DEBUG

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
#if DEBUG == 1
	printf("MOSI set to: %d\n", bit);
#endif
}

static inline void toggle_sck(struct spi *spi)
{
	*(spi->sck_addr) ^= 0x1;
#if DEBUG == 1
	printf("SPI clock toggled\n");
#endif
}

static inline void set_ss(struct spi *spi)
{
	*(spi->ss_addr) = 0x1;
#if DEBUG == 1
	printf("Slave select activated\n");
#endif
}

static inline void clear_ss(struct spi *spi)
{
	*(spi->ss_addr) = 0x0;
#if DEBUG == 1
	printf("Slave select cleared\n");
#endif
}

static inline void set_sck(struct spi *spi)
{
	*(spi->sck_addr) = 0x1;
#if DEBUG == 1
	printf("SCK activated\n");
#endif
}

static inline void clear_sck(struct spi *spi)
{
	*(spi->sck_addr) = 0x0;
#if DEBUG == 1
	printf("SCK cleared\n");
#endif
}

/* Protocolo:
 * Byte 1 -> Comando -> Retorno FPGA [7-6] | Operação [5-2] | Canal da image [1-0]
 * Bytes 2-3 -> Altura da imagem
 * Bytes 3-4 -> Largura da imagem
 * Bytes restantes -> Pixels da imagem
 *
 * Retorno FPGA: 00 -> Sem retorno | 01 -> PDI em execução,
 *
 * Operação: 0000 -> Nenhuma operação | 0001 -> Envio de imagem | 0010 -> Recebimento de
 * imagem | 0011 -> Execução de PDI,
 *
 * Canal da imagem: 00 -> Canal padrão (R) | 01 -> Canal 1 (R) | 02 -> Canal 2(G) |
 * 11 -> Canal 3 (B).
 */

#define NO_RETURN_MASK   0b00000000
#define PDI_RUNNING_MASK 0b01000000

#define NO_OP_MASK         0b00000000
#define SEND_IMAGE_OP_MASK 0b00000100
#define RECV_IMAGE_OP_MASK 0b00001000
#define PDI_EXEC_OP_MASK   0b00001100

#define IMAGE_DFT_CHN 0b00000000
#define IMAGE_CHN_R   0b00000001
#define IMAGE_CHN_G   0b00000010
#define IMAGE_CHN_B   0b00000011

#define IMG_HEIGHT 3
#define IMG_WIDTH  3

uint8_t r_channel[IMG_HEIGHT * IMG_WIDTH] = {
	0x00, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0x00,
}; // Exemplo de array de bytes a serem enviados
uint8_t g_channel[IMG_HEIGHT * IMG_WIDTH] = {
	0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF,
}; // Exemplo de array de bytes a serem enviados

uint8_t b_channel[IMG_HEIGHT * IMG_WIDTH] = {
	0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF,
}; // Exemplo de array de bytes a serem enviados
void bringup_sequence(struct spi *spi, uint8_t multiplier)
{
	// Testa a sequência de inicialização do SPI

	usleep(multiplier *
	       ONE_SECOND_US); // Pequeno atraso entre bytes, se DEBUG estiver habilitado
	set_sck(spi);          // Ativa o clock SPI

	usleep(multiplier *
	       ONE_SECOND_US); // Pequeno atraso entre bytes, se DEBUG estiver habilitado
	set_mosi(spi, 0x1);    // Ativa o MOSI

	usleep(multiplier *
	       ONE_SECOND_US); // Pequeno atraso entre bytes, se DEBUG estiver habilitado

	clear_sck(spi); // Desativa o clock SPI
	usleep(multiplier *
	       ONE_SECOND_US); // Pequeno atraso entre bytes, se DEBUG estiver habilitado

	set_mosi(spi, 0x0); // Desativa o MOSI
	usleep(multiplier *
	       ONE_SECOND_US); // Pequeno atraso entre bytes, se DEBUG estiver habilitado

	clear_ss(spi); // Seleciona o slave
	usleep(multiplier *
	       ONE_SECOND_US); // Pequeno atraso entre bytes, se DEBUG estiver habilitado

	// Inicializa o slave select
	set_ss(spi); // Libera o slave após a transferência
	usleep(multiplier *
	       ONE_SECOND_US); // Pequeno atraso entre bytes, se DEBUG estiver habilitado
}

static inline void spi_change_to_default(struct spi *spi)
{
	*(spi->mosi_addr) = 0x0;
	*(spi->sck_addr) = 0x0;
	*(spi->ss_addr) = 0x1;
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

#if DEBUG == 1
	printf("Endereco virtual do bridge: 0x%X\n", (unsigned int)virtual_base);
#endif

	spi->miso_addr = virtual_base + ((unsigned long)(ALT_LWFPGASLVS_OFST + SPI_MISO_BASE) &
					 (unsigned long)(HW_REGS_MASK));
	spi->mosi_addr = virtual_base + ((unsigned long)(ALT_LWFPGASLVS_OFST + SPI_MOSI_BASE) &
					 (unsigned long)(HW_REGS_MASK));
	spi->sck_addr = virtual_base + ((unsigned long)(ALT_LWFPGASLVS_OFST + SPI_SCK_BASE) &
					(unsigned long)(HW_REGS_MASK));
	spi->ss_addr = virtual_base + ((unsigned long)(ALT_LWFPGASLVS_OFST + SPI_SS_BASE) &
				       (unsigned long)(HW_REGS_MASK));

	spi_change_to_default(spi);
	return 0;
}

// Função para transmitir um byte via SPI
void spi_send_byte(struct spi *spi, uint8_t byte)
{
	printf("Enviando byte: " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(byte));
	printf("\n");

	clear_ss(spi);                                   // Seleciona o slave
	for (int i = 7; i >= 0; i--) {                   // SPI é geralmente MSB first
		clear_sck(spi);                          // Troca o clock
		usleep(SPI_COMM_PERIOD * ONE_SECOND_US); // Espera o período do clock SPI

		set_mosi(spi, (byte >> i) & 0x1);        // Envia o bit atual
		set_sck(spi);                            // Troca de volta o clock
		usleep(SPI_COMM_PERIOD * ONE_SECOND_US); // Espera o período do clock SPI
	}

	spi_change_to_default(spi); // Volta para o estado inicial
}

// Função para receber um byte via SPI (leitura do pino MISO)
uint8_t spi_receive_byte(struct spi *spi)
{
	uint8_t received_byte = 0;

	clear_ss(spi);                                   // Seleciona o slave
	for (int i = 7; i >= 0; i--) {                   // SPI é geralmente MSB first
		clear_sck(spi);                          // Troca o clock
		usleep(SPI_COMM_PERIOD * ONE_SECOND_US); // Espera o período do clock SPI

		// Lê o bit de MISO e insere no byte recebido

		set_sck(spi);                                   // Troca o clock de volta
		usleep((SPI_COMM_PERIOD >> 1) * ONE_SECOND_US); // Espera o período do clock SPI
		uint8_t bit = (*(spi->miso_addr) & 0x1);
		usleep((SPI_COMM_PERIOD >> 1) * ONE_SECOND_US); // Espera o período do clock SPI

		received_byte |= (bit << i);
	}

	spi_change_to_default(spi); // Volta para o estado inicial

#if DEBUG == 1
	printf("Byte recebido: 0x%X\n", received_byte);
#endif

	return received_byte;
}

void fill_data_to_send(uint8_t *data_to_send, uint8_t start_byte, uint8_t *img_data)
{

	data_to_send[0] = start_byte;
	data_to_send[1] = GET_MSB_16BIT(IMG_HEIGHT);
	data_to_send[2] = GET_LSB_16BIT(IMG_HEIGHT);
	data_to_send[3] = GET_MSB_16BIT(IMG_WIDTH);
	data_to_send[4] = GET_LSB_16BIT(IMG_WIDTH);

	for (int i = 0; i < (IMG_HEIGHT * IMG_WIDTH); i++) {
		data_to_send[5 + i] = img_data[i];
	}
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

#if DEBUG == 1
	printf("DEBUG habilitado!\n");
	bringup_sequence(&spi_fields, 1);
#endif
	uint8_t start_byte_r_ch = NO_RETURN_MASK | SEND_IMAGE_OP_MASK | IMAGE_CHN_R;
	uint8_t data_to_send_r_ch[5 + (IMG_HEIGHT * IMG_WIDTH)];
	uint8_t data_to_send_g_ch[5 + (IMG_HEIGHT * IMG_WIDTH)];
	uint8_t data_to_send_b_ch[5 + (IMG_HEIGHT * IMG_WIDTH)];
	uint8_t received_byte = 0;

	fill_data_to_send(data_to_send_r_ch, start_byte_r_ch, r_channel);
	fill_data_to_send(data_to_send_g_ch, start_byte_r_ch, g_channel);
	fill_data_to_send(data_to_send_b_ch, start_byte_r_ch, b_channel);

	size_t data_len = sizeof(data_to_send_r_ch) / sizeof(data_to_send_r_ch[0]);

	for (size_t i = 0; i < data_len; i++) {
		spi_send_byte(&spi_fields, data_to_send_r_ch[i]); // Envia o byte
	}

	usleep(2 * ONE_SECOND_US); // Pequeno atraso entre bytes, se DEBUG estiver habilitado
	uint8_t request_image = NO_RETURN_MASK | RECV_IMAGE_OP_MASK | IMAGE_CHN_R;

	printf("Recebendo imagem do canal R\n");
	spi_send_byte(&spi_fields, request_image); // Envia o byte
	for (size_t i = 0; i < 100000; i++) {
		received_byte = spi_receive_byte(&spi_fields); // Recebe o byte
		if (i % 1000 == 0) {
			printf("Iteracao %d\n", i);
		}
	}

	UNUSED(request_image);
	UNUSED(received_byte);
	printf("Transferencia de dados concluida!\n");

	return 0;
}
