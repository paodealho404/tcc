
# Projeto de Processamento Colaborativo ARM-FPGA

Este projeto visa explorar o **processamento colaborativo ARM-FPGA** com o uso do **Cyclone V SoC** (contendo o processador ARM Cortex-A9) e o **Raspberry Pi** para tarefas de **reconhecimento de gestos**. O objetivo é integrar esses diferentes componentes para realizar o processamento colaborativo de sinais e dados de forma eficiente.

## Estrutura do Projeto

O projeto está organizado nas seguintes pastas:

### 1. `hps` (Hard Processor System)

Contém o código e scripts relacionados ao **processador ARM Cortex-A9** presente no **Cyclone V SoC**. O conteúdo dessa pasta envolve o desenvolvimento de software que roda no processador ARM, incluindo bibliotecas, e algoritmos de processamento que colaboram com a FPGA para tarefas de reconhecimento de gestos.

- **Conteúdo**:
  - Código C/C++ para processamento das informações no ARM
  - Drivers e bibliotecas para comunicação com a FPGA
  - Scripts para configuração do ambiente de desenvolvimento no ARM

### 2. `fpga` (Field-Programmable Gate Array)

Contém a lógica customizada implementada na **FPGA Cyclone V**. Essa pasta inclui os arquivos de configuração e o código Verilog ou VHDL para definir a funcionalidade da FPGA, que pode envolver operações como processamento de sinais, reconhecimento de padrões, ou outras tarefas que complementam o processamento realizado pelo ARM.

- **Conteúdo**:
  - Código Verilog/VHDL para a implementação da lógica na FPGA
  - Arquivos de configuração (.bit) para programação da FPGA
  - Scripts para geração de arquivos e envio do executável para a placa

### 3. `raspberry`

Contém os scripts, arquivos gerais e códigos relacionados ao uso do **Raspberry Pi** no projeto. O Raspberry Pi é utilizado como um controlador externo. Nessa pasta, encontram-se os códigos que permitem a comunicação entre o Raspberry Pi e a FPGA, além de possíveis interfaces para o reconhecimento de gestos.

- **Conteúdo**:
  - Scripts para interação entre Raspberry Pi e FPGA
  - Códigos para processamento das mensagens e controle de dispositivos conectados ao Raspberry Pi (Cyclone V SoC)

## Como Executar

### Requisitos

- **Cyclone V SoC** com o processador ARM Cortex-A9
- **Raspberry Pi** (qualquer modelo que tenha suporte para comunicação com o SoC)
- Ambiente de desenvolvimento com ferramentas para programação de FPGA (e.g., Quartus II) e compilação para o ARM (e.g., GCC ARM EabiHF )
  - Conexões de rede entre o SoC e máquina de desenvolvimento
  - Quartus II
  - Altera EDS
  - arm-none-eabihf-gcc
  - Python 3.10+

### Passos

1. **Configuração do HPS (ARM Cortex-A9)**:

   - Navegue até a pasta `hps` e siga os passos para configurar o software no ARM.
   - Compile o código e faça o upload para o ARM usando as ferramentas apropriadas.
   - Utilize o executável na própria HPS para estabelecer a colaboração com o Cortex-A9 e a FPGA, realizando as tarefas de reconhecimento de gestos.
2. **Configuração da FPGA**:

   - Navegue até a pasta `fpga` e utilize o Quartus II ou outra ferramenta de desenvolvimento para compilar e programar a FPGA.
   - Verifique se a lógica personalizada está funcionando corretamente com os testes fornecidos.
3. **Configuração do Raspberry Pi**:

   - Navegue até a pasta `raspberry` e siga as instruções para configurar a comunicação entre o Raspberry Pi e os outros componentes.
   - Execute os scripts no Raspberry Pi para estabelecer a colaboração com o Raspberry e a FPGA, realizando as tarefas de reconhecimento de gestos.

## Contribuição

Se você deseja contribuir para o projeto, por favor, faça um fork deste repositório e envie um pull request com suas melhorias ou correções. Certifique-se de testar seu código antes de submeter.

## Licença

Este projeto está licenciado sob a [MIT License](LICENSE).
