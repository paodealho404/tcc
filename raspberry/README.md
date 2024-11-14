# Raspberry Python code

Sends an image to the FPGA, where it is stored, and later retrieves it upon request.

## SPI Connection

| PIN | RPi5 | FPGA |
| ------------- | ------------- | ------------- |
| MOSI | GPIO 10 | GPIO_0_D1 |
| MISO | GPIO 9 | GPIO_0_D0 |
| SCK | GPIO 11 | GPIO_0_D2 |
| SS | GPIO 22 | GPIO_0_D3 |
| GND | PIN 20 | PIN 12 |

![image](https://github.com/gustavo95/DOC_PP1_RASP/assets/7265988/7ce6863b-e8e2-4029-89d7-436c77835f90)

<img width="587" alt="image" src="https://github.com/gustavo95/DOC_PP1_RASP/assets/7265988/eb4ba98a-2803-453f-816b-9b6074c9c710">
