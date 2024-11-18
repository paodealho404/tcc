#include "pdi.h"

int execute_pdi()
{
	uint8_t start_pdi_byte = NO_RETURN_MASK | PDI_EXEC_OP_MASK;
	uint8_t received_byte = 0;

	// clock_gettime(CLOCK_REALTIME, &start_time);

	spi_send_byte(0x00);           // Envia o byte
	spi_send_byte(start_pdi_byte); // Envia o byte

	while (1) {
		received_byte = spi_receive_byte(); // Recebe o byte
		if (received_byte == 0x00) {
			continue;
		}
		break;
	}

	while (1) {
		received_byte = spi_receive_byte(); // Recebe o byte
		if (received_byte == PDI_RUNNING_MASK) {
			continue;
		}
		break;
	}

	uint8_t gesture_eval = NO_RETURN_MASK | GESTURE_EVAL_MASK | IMAGE_CHN_DFT;

	// clock_gettime(CLOCK_REALTIME, &start_time);

	spi_send_byte(0x00);         // Envia o byte
	spi_send_byte(gesture_eval); // Envia o byte
	spi_send_byte(0x00);         // Envia o byte

	uint32_t pdi_result = 0;
	for (int i = 3; i >= 0; i--) {
		uint8_t received_byte = spi_receive_byte();
		pdi_result |= (received_byte << (8 * i));
	}

	switch (pdi_result) {
	case 1:
#if DEBUG == 1
		printf("\nClassification: One Finger Up\n");
#endif
		break;
	case 2:
#if DEBUG == 1
		printf("\nClassification: Two Fingers Up\n");
#endif
		break;

	case 3:
#if DEBUG == 1
		printf("\nClassification: Three Fingers Up\n");
#endif
		break;
	case 4:
#if DEBUG == 1
		printf("\nClassification: Four Fingers Up\n");
#endif
		break;
	case 5:
#if DEBUG == 1
		printf("\nClassification: Open Palm\n");
#endif
		break;
	case 6:
#if DEBUG == 1
		printf("\nClassification: Closed Fist\n");
#endif
		break;
	default:
		printf("\nClassification: Unknown %u\n", pdi_result);
		break;
	}

#if DEBUG == 1
	uint8_t img_r_start_byte = NO_RETURN_MASK | RECV_IMAGE_OP_MASK | IMAGE_CHN_R;
	uint16_t img_white = 0;
	spi_send_byte(0x00);             // Envia o byte
	spi_send_byte(img_r_start_byte); // Envia o byte
	spi_send_byte(0x00);             // Envia o byte

	// Cria arquivo com a imagem em formato de texto
	FILE *img_file = fopen("img_r_channel.txt", "w");
	if (img_file == NULL) {
		printf("Erro ao criar o arquivo\n");
		return -1;
	}

	for (size_t i = 0; i < IMG_HEIGHT * IMG_WIDTH; i++) {
		uint8_t received_byte = spi_receive_byte(); // Recebe o byte
		fprintf(img_file, "%c", received_byte ? '*' : ' ');
		if (i % 320 == 0 && i != 0) {
			fprintf(img_file, "\n");
		}

		if (received_byte > 0) {

			img_white++;
		}
	}
#endif

	uint32_t hand_area_result = 0;
	spi_send_byte(HAND_AREA_MASK);
	spi_send_byte(0x00); // Envia o byte

	for (int i = 3; i >= 0; i--) {
		uint8_t received_byte = spi_receive_byte();
		hand_area_result |= (received_byte << (8 * i));
	}

#if DEBUG == 1
	printf("\nHand area: %d\n", hand_area_result);
#endif

	uint32_t hand_per_result = 0;
	spi_send_byte(HAND_PER_MASK);
	spi_send_byte(0x00); // Envia o byte

	for (int i = 3; i >= 0; i--) {
		uint8_t received_byte = spi_receive_byte();
		hand_per_result |= (received_byte << (8 * i));
	}

#if DEBUG == 1
	printf("\nHand perimeter: %d\n", hand_per_result);
#endif

	uint32_t hand_peak_result = 0;
	spi_send_byte(HAND_PEAK_MASK);
	spi_send_byte(0x00); // Envia o byte

	for (int i = 3; i >= 0; i--) {
		uint8_t received_byte = spi_receive_byte();
		hand_peak_result |= (received_byte << (8 * i));
	}

#if DEBUG == 1
	printf("\nHand peak: %d\n", hand_peak_result);
#endif
	return 0;
}