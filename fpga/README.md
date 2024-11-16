# FPGA Verilog Code

Receives an image, stores it in BRAM and then sends it back.

## SPI Connection

| PIN  | RPi5    | FPGA      |
| ---- | ------- | --------- |
| MOSI | GPIO 10 | GPIO_0_D1 |
| MISO | GPIO 9  | GPIO_0_D0 |
| SCK  | GPIO 11 | GPIO_0_D2 |
| SS   | GPIO 7  | GPIO_0_D3 |
| GND  | PIN 20  | PIN 12    |

![image](https://github.com/gustavo95/DOC_PP1_RASP/assets/7265988/7ce6863b-e8e2-4029-89d7-436c77835f90)

<img width="587" alt="image" src="https://github.com/gustavo95/DOC_PP1_RASP/assets/7265988/eb4ba98a-2803-453f-816b-9b6074c9c710">






# Pipeline de Reconhecimento de Gestos

Este diagrama descreve as etapas sequenciais de processamento da pipeline utilizada para o reconhecimento de gestos, conforme o modelo descrito por Gupta, Sehrawat e Khosla (2012).

```mermaid
graph TD
    A[Recebimento dos canais RGB da imagem] --> Y[Início PDI]
	Y -->     B[Cálculo das Médias dos Canais de Cor]
    B --> C[Cálculo do Máximo das Médias e Compensação de Iluminação]
    C --> D[Conversão de RGB para YCbCr]
    D --> E[Binarização]
    E --> F[Erosão e Dilatação]
    F --> G[Extração das Features]
    G --> H[Solicitação de inferência]
    G --> I[Solitação de uma feature específica]

classDef default fill:#e2f0f9,stroke:#4f8cc9,stroke-width:3px;
```
