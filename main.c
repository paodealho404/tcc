#include "image.h"
#include "spi.h"
#include "pdi.h"

#define GET_MSB_16BIT(x) ((uint8_t)((x) >> 8))
#define GET_LSB_16BIT(x) ((uint8_t)((x) & 0xFF))

void fill_data_to_send(uint8_t *data_to_send, uint8_t start_byte, const uint8_t *img_data)
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

	printf("Iniciando a transferencia de dados via SPI!\n");

	int err = 0;

	err = setup_mem_addr(); // Assumimos que esta função está correta
	if (err) {
		printf("Erro ao configurar os enderecos de memoria\n");
		return err;
	}

#if DEBUG == 1
	printf("DEBUG habilitado!\n");
	bringup_sequence();
#endif
	uint8_t start_byte_r_ch = NO_RETURN_MASK | SEND_IMAGE_OP_MASK | IMAGE_CHN_R;
	uint8_t start_byte_g_ch = NO_RETURN_MASK | SEND_IMAGE_OP_MASK | IMAGE_CHN_G;
	uint8_t start_byte_b_ch = NO_RETURN_MASK | SEND_IMAGE_OP_MASK | IMAGE_CHN_B;

	uint8_t image_r_ch_pkt[5 + (IMG_HEIGHT * IMG_WIDTH)];
	uint8_t image_g_ch_pkt[5 + (IMG_HEIGHT * IMG_WIDTH)];
	uint8_t image_b_ch_pkt[5 + (IMG_HEIGHT * IMG_WIDTH)];
	uint8_t received_byte = 0;

	UNUSED(received_byte);

	fill_data_to_send(image_r_ch_pkt, start_byte_r_ch, img_r_channel);
	fill_data_to_send(image_g_ch_pkt, start_byte_g_ch, img_g_channel);
	fill_data_to_send(image_b_ch_pkt, start_byte_b_ch, img_b_channel);

	size_t data_len = sizeof(image_r_ch_pkt) / sizeof(image_r_ch_pkt[0]);

	for (size_t i = 0; i < 10; i++) {
		printf("0x%X 0x%X 0x%X 0x%X 0x%X \t", image_r_ch_pkt[i], image_r_ch_pkt[i + 1],
		       image_r_ch_pkt[i + 2], image_r_ch_pkt[i + 3], image_r_ch_pkt[i + 4]);
		printf("0x%X 0x%X 0x%X 0x%X 0x%X \t", image_g_ch_pkt[i], image_g_ch_pkt[i + 1],
		       image_g_ch_pkt[i + 2], image_g_ch_pkt[i + 3], image_g_ch_pkt[i + 4]);
		printf("0x%X 0x%X 0x%X 0x%X 0x%X \n", image_b_ch_pkt[i], image_b_ch_pkt[i + 1],
		       image_b_ch_pkt[i + 2], image_b_ch_pkt[i + 3], image_b_ch_pkt[i + 4]);
	}

	// clock_gettime(CLOCK_REALTIME, &start_time);
	start_time = time(NULL);

	for (size_t i = 0; i < data_len; i++) {
		spi_send_byte(image_r_ch_pkt[i]); // Envia o byte
	}

	printf("Tempo de envio canal vermelho: %lf\n", difftime(time(NULL), start_time));

	start_time = time(NULL);
	spi_send_byte(0x00); // Envia o byte

	for (size_t i = 0; i < data_len; i++) {
		spi_send_byte(image_g_ch_pkt[i]); // Envia o byte
	}

	// clock_gettime(CLOCK_REALTIME, &send_time);
	printf("Tempo de envio canal verde: %lf\n", difftime(time(NULL), start_time));

	// clock_gettime(CLOCK_REALTIME, &start_time);
	start_time = time(NULL);
	spi_send_byte(0x00); // Envia o byte

	for (size_t i = 0; i < data_len; i++) {
		spi_send_byte(image_b_ch_pkt[i]); // Envia o byte
	}

	// clock_gettime(CLOCK_REALTIME, &send_time);
	printf("Tempo de envio canal azul: %lf\n", difftime(time(NULL), start_time));

	err = execute_pdi();
	if (err) {
		printf("Erro ao executar o PDI\n");
		return -1;
	}

	printf("\nTransferencia de dados concluida!\n");
	return 0;
}
