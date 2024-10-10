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
#include "image.h"
#include <time.h>

// Configurações para o bridge HPS-to-FPGA
#define HW_REGS_BASE (ALT_STM_OFST)
#define HW_REGS_SPAN (0x04000000) // 64 MB com espaço de endereçamento de 32 bits
#define HW_REGS_MASK (HW_REGS_SPAN - 1)

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)                                                                       \
	((byte) & 0x80 ? '1' : '0'), ((byte) & 0x40 ? '1' : '0'), ((byte) & 0x20 ? '1' : '0'),     \
		((byte) & 0x10 ? '1' : '0'), ((byte) & 0x08 ? '1' : '0'),                          \
		((byte) & 0x04 ? '1' : '0'), ((byte) & 0x02 ? '1' : '0'),                          \
		((byte) & 0x01 ? '1' : '0')

#define GET_MSB_16BIT(x) ((uint8_t)((x) >> 8))
#define GET_LSB_16BIT(x) ((uint8_t)((x) & 0xFF))
#define UNUSED(x)        (void)(x)
#define ONE_SECOND_NS    (1000000000)

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

static inline void set_sck(struct spi *spi)
{
	*(spi->sck_addr) = 0x1;
}

static inline void clear_sck(struct spi *spi)
{
	*(spi->sck_addr) = 0x0;
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

#define IMG_HEIGHT         240
#define IMG_WIDTH          320
#define DELAY_MAGIC_NUMBER 1

int bringup_sequence(struct spi *spi, uint8_t multiplier)
{
	// Testa a sequência de inicialização do SPI

	set_sck(spi); // Ativa o clock SPI

	set_mosi(spi, 0x1); // Ativa o MOSI

	clear_sck(spi); // Desativa o clock SPI

	set_mosi(spi, 0x0); // Desativa o MOSI

	clear_ss(spi); // Seleciona o slave

	// Inicializa o slave select
	set_ss(spi); // Libera o slave após a transferência

	return 0;
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
void delay(uint32_t cyc)
{
	uint32_t cycles = cyc;
	while (cycles--) {
	}
}
// Função para transmitir um byte via SPI
void spi_send_byte(struct spi *spi, uint8_t byte)
{
	clear_ss(spi);                 // Seleciona o slave
	for (int i = 7; i >= 0; i--) { // SPI é geralmente MSB first
		clear_sck(spi);
		delay(DELAY_MAGIC_NUMBER); // Troca o clock

		set_mosi(spi, (byte >> i) & 0x1); // Envia o bit atual
		set_sck(spi);                     // Troca de volta o clock
		delay(DELAY_MAGIC_NUMBER);
	}
	spi_change_to_default(spi); // Volta para o estado inicial
}

// Função para receber um byte via SPI (leitura do pino MISO)
uint8_t spi_receive_byte(struct spi *spi)
{
	uint8_t received_byte = 0;

	clear_ss(spi);                 // Seleciona o slave
	for (int i = 7; i >= 0; i--) { // SPI é geralmente MSB first
		clear_sck(spi);        // Troca o clock
		delay(DELAY_MAGIC_NUMBER);

		set_sck(spi); // Troca o clock de volta
		uint8_t bit = (*(spi->miso_addr) & 0x1);
		delay(DELAY_MAGIC_NUMBER);

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
	time_t start_time = {0};

	struct spi spi_fields; // Estrutura do SPI inicializada
	initializeImage();

	printf("Iniciando a transferencia de dados via SPI!\n");

	int err = 0;

	err = setup_mem_addr(&spi_fields); // Assumimos que esta função está correta
	if (err) {
		printf("Erro ao configurar os enderecos de memoria\n");
		return err;
	}

#if DEBUG == 1
	printf("DEBUG habilitado!\n");
	bringup_sequence(&spi_fields, 5);
#endif
	uint8_t start_byte_r_ch = NO_RETURN_MASK | SEND_IMAGE_OP_MASK | IMAGE_CHN_R;
	uint8_t start_byte_g_ch = NO_RETURN_MASK | SEND_IMAGE_OP_MASK | IMAGE_CHN_G;
	uint8_t start_byte_b_ch = NO_RETURN_MASK | SEND_IMAGE_OP_MASK | IMAGE_CHN_B;

	uint8_t image_r_ch[5 + (IMG_HEIGHT * IMG_WIDTH)];
	uint8_t image_g_ch[5 + (IMG_HEIGHT * IMG_WIDTH)];
	uint8_t image_b_ch[5 + (IMG_HEIGHT * IMG_WIDTH)];
	uint8_t received_byte = 0;

	fill_data_to_send(image_r_ch, start_byte_r_ch, image);
	fill_data_to_send(image_g_ch, start_byte_g_ch, image);
	fill_data_to_send(image_b_ch, start_byte_b_ch, image);

	size_t data_len = sizeof(image_r_ch) / sizeof(image_r_ch[0]);

	// clock_gettime(CLOCK_REALTIME, &start_time);
	start_time = time(NULL);

	for (size_t i = 0; i < data_len; i++) {
		spi_send_byte(&spi_fields, image_r_ch[i]); // Envia o byte
	}

	printf("Tempo de envio canal vermelho: %lf\n", difftime(time(NULL), start_time));

	start_time = time(NULL);
	spi_send_byte(&spi_fields, 0x00); // Envia o byte

	for (size_t i = 0; i < data_len; i++) {
		spi_send_byte(&spi_fields, image_g_ch[i]); // Envia o byte
	}

	// clock_gettime(CLOCK_REALTIME, &send_time);
	printf("Tempo de envio canal verde: %lf\n", difftime(time(NULL), start_time));

	// clock_gettime(CLOCK_REALTIME, &start_time);
	start_time = time(NULL);
	spi_send_byte(&spi_fields, 0x00); // Envia o byte

	for (size_t i = 0; i < data_len; i++) {
		spi_send_byte(&spi_fields, image_b_ch[i]); // Envia o byte
	}

	// clock_gettime(CLOCK_REALTIME, &send_time);
	printf("Tempo de envio canal azul: %lf\n", difftime(time(NULL), start_time));

	uint8_t request_image = NO_RETURN_MASK | RECV_IMAGE_OP_MASK | IMAGE_CHN_R;

	uint8_t image_rcvd[IMG_WIDTH * IMG_HEIGHT] = {0};

	printf("Recebendo imagem do canal R\n");

	// clock_gettime(CLOCK_REALTIME, &start_time);
	start_time = time(NULL);
	spi_send_byte(&spi_fields, 0x00);          // Envia o byte
	spi_send_byte(&spi_fields, request_image); // Envia o byte

	spi_receive_byte(&spi_fields);
	for (size_t i = 0; i < (IMG_WIDTH * IMG_HEIGHT); i++) {
		image_rcvd[i] = spi_receive_byte(&spi_fields); // Recebe o byte
	}

	// clock_gettime(CLOCK_REALTIME, &recv_time);
	printf("Tempo de recebimento canal vermelho: %lf\n", difftime(time(NULL), start_time));

	for (size_t i = 0; i < IMG_WIDTH; i++) {
		printf("0x%X ", image_rcvd[i]);
		if ((i + 1) % IMG_WIDTH / 10 == 0) {
			printf("\n");
		}
	}

	UNUSED(request_image);
	UNUSED(received_byte);
	printf("Transferencia de dados concluida!\n");

	return 0;
}
