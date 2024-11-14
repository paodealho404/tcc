/*
 * Module Name: top.
 *
 * Description: Verilog "main" module.
 *
 * Inputs:
 *    clk - Main clock signal from oscillator (AF14)
 *    rst - Reset signal from button key0 (AA14)
 *    ss - Chip select signal from SPI (Y18)
 *    mosi - Master out slave in signal from SPI (Y17)
 *    sck - Communication clock signal from SPI (AD17)
 *
 * Outputs:
 *    miso - Master in slave out signal to SPI (AC18)
 *    hex0 - 7-segment display 0 (AE26, AE27, AE28, AG27, AF28, AG28, AH28)
 *    hex1 - 7-segment display 1 (AJ29, AH29, AH30, AG30, AF29, AF30, AD27)
 * 	hex4 - 7-segment display 4 ,
 *	   hex5 - 7-segment display 5,
 *    led0 - LED 0 (V16)
 *    led1 - LED 1 (W16)
 *    led2 - LED 2 (V17)
 *    led3 - LED 3 (V18)
 *    led4 - LED 4 (W17)
 *    led5 - LED 5 (W19)
 *    led6 - LED 6 (Y19)
 *    led7 - LED 7 (W20)
 *    led8 - LED 8 (W21)
 *
 * Functionality:
 *    Connect the modules to each other.
 *    Define the inputs and outputs of the sistem.
 */

module top (
	//Control
	input clk,
	input rst,
	
	//SPI
	input ss,
	input mosi,
	output miso,
	input sck,
	
	//7segments
	output [6:0] hex0,
	output [6:0] hex1,
	
	output [6:0] hex4,
	output [6:0] hex5,
	
	//led
	output led0,
	output led1,
	output led2,
	output led3,
	output led4,
	output led5,
	output led6,
	output led7,
	output led8,
	output led9,

    // HPS SIDE
    inout  HPS_CONV_USB_N,
    output [14:0] HPS_DDR3_ADDR,
    output [2:0] HPS_DDR3_BA,
    output HPS_DDR3_CAS_N,
    output HPS_DDR3_CKE,
    output HPS_DDR3_CK_N,
    output HPS_DDR3_CK_P,
    output HPS_DDR3_CS_N,
    output [3:0] HPS_DDR3_DM,
    inout  [31:0] HPS_DDR3_DQ,
    inout  [3:0] HPS_DDR3_DQS_N,
    inout  [3:0] HPS_DDR3_DQS_P,
    output HPS_DDR3_ODT,
    output HPS_DDR3_RAS_N,
    output HPS_DDR3_RESET_N,
    input  HPS_DDR3_RZQ,
    output HPS_DDR3_WE_N,
    output HPS_ENET_GTX_CLK,
    inout  HPS_ENET_INT_N,
    output HPS_ENET_MDC,
    inout  HPS_ENET_MDIO,
    input  HPS_ENET_RX_CLK,
    input  [3:0] HPS_ENET_RX_DATA,
    input  HPS_ENET_RX_DV,
    output [3:0] HPS_ENET_TX_DATA,
    output HPS_ENET_TX_EN,
    inout  HPS_KEY,
    output HPS_SD_CLK,
    inout  HPS_SD_CMD,
    inout  [3:0] HPS_SD_DATA,
    input  HPS_UART_RX,
    output HPS_UART_TX,
    input  HPS_USB_CLKOUT,
    inout  [7:0] HPS_USB_DATA,
    input  HPS_USB_DIR,
    input  HPS_USB_NXT,
    output HPS_USB_STP
);

	wire not_used_hsf;
	wire fpga_mosi;
    wire fpga_miso;
    wire fpga_s0;
    wire fpga_sck;

	wire [2:0] state;

	// SPI wires
	wire spi_cycle_done;
	wire [7:0] data_to_send;
	wire [7:0] data_received;
	
	// BRAM wires
	wire com_we;
	wire pdi_we;
	wire [1:0] bram_channel;
	wire [7:0] bram_data_in;
	wire [7:0] bram_data_out;
	wire [16:0] com_addr;
	wire [16:0] pdi_addr_read;
	wire [16:0] pdi_addr_write;

	// Image processing wires
	wire pdi_active;
	wire pdi_done;
	wire [7:0] red_data_in;
	wire [7:0] green_data_in;
	wire [7:0] blue_data_in;
	wire [7:0] red_data_out;
	wire [7:0] green_data_out;
	wire [7:0] blue_data_out;
	wire [16:0] hand_area;
	wire [16:0] hand_perimeter;
	wire [34:0] max_distance;
	wire [9:0] peaks;
	wire [3:0] classification;
	
	// LEDs assignments
	assign led0 = state[0];
	assign led1 = state[1];
	assign led2 = state[2];
	assign led3 = pdi_addr_write[0];
	assign led4 = pdi_addr_write[1];
	assign led5 = pdi_addr_write[2];
	assign led6 = fpga_miso;
	assign led7 = fpga_mosi;
	assign led8 = fpga_s0;
	assign led9 = fpga_sck;
	
	// 7-segments modules
	segment7 segment_seven_0 (
		.bcd(data_received[3:0]), // Mudar para data received e testar
		.seg(hex0)
	);
	
	segment7 segment_seven_1 (
		.bcd(data_received[7:4]),
		.seg(hex1)
	);
	
	segment7 segment_seven_4 (
		.bcd(data_to_send[3:0]),
		.seg(hex4)
	);
	
	segment7 segment_seven_5 (
		.bcd(data_to_send[7:4]),
		.seg(hex5)
	);


	// Image processing modules
	img_processing img_proc (
		.clk(clk),
		.rst(rst),
		.active(pdi_active),
		.done(pdi_done),
		.red_data_in(red_data_out),
		.green_data_in(green_data_out),
		.blue_data_in(blue_data_out),
		.red_data_out(red_data_in),
		.green_data_out(green_data_in),
		.blue_data_out(blue_data_in),
		.we(pdi_we),
		.addr_read(pdi_addr_read),
		.addr_write(pdi_addr_write),
		.hand_area(hand_area),
		.hand_perimeter(hand_perimeter),
		.max_distance(max_distance),
		.peaks(peaks),
		.classification(classification)
	);
	
	// Storage modules
	bram_controller bram_ctrl (
		.clk(clk),
		.com_addr(com_addr),
		.pdi_addr_read(pdi_addr_read),
		.pdi_addr_write(pdi_addr_write),
		.channel(bram_channel),
		.com_we(com_we),
		.pdi_we(pdi_we),
		.pdi_active(pdi_active),
		.data_in(bram_data_in),
		.data_out(bram_data_out),
		.red_data_in(red_data_in),
		.green_data_in(green_data_in),
		.blue_data_in(blue_data_in),
		.red_data_out(red_data_out),
		.green_data_out(green_data_out),
		.blue_data_out(blue_data_out)
	);
	
	// Communication modules
	data_transfer_controller dtc (
		.clk(clk),
		.rst(rst),
		.spi_cycle_done(spi_cycle_done),
		.spi_byte_in(data_received),
		.spi_byte_out(data_to_send),
		.bram_addr(com_addr),
		.bram_channel(bram_channel),
		.bram_we(com_we),
		.bram_data_in(bram_data_in),
		.bram_data_out(bram_data_out),
		.pdi_active(pdi_active),
		.pdi_done(pdi_done),
		.hand_area(hand_area),
		.hand_perimeter(hand_perimeter),
		.state(state),
		.max_distance(max_distance),
		.peaks(peaks),
		.classification(classification)
	);
	
//	spi_slave_2 spi(
//		.i_Rst_L(rst),
//		.i_Clk(clk),
//		.o_RX_DV(spi_cycle_done),
//		.o_RX_Byte(data_received),
//		.i_TX_DV(1'b1),
//		.i_TX_Byte(data_to_send),
//		.i_SPI_Clk(fpga_sck),
//		.o_SPI_MISO(fpga_miso),
//		.i_SPI_MOSI(fpga_mosi),
//		.i_SPI_CS_n(fpga_s0)
//	);

 spi_slave spi(
 	.clk(clk),
 	.rst(rst),
 	.ss(fpga_s0),
 	.mosi(fpga_mosi),
 	.miso(fpga_miso),
 	.sck(fpga_sck),
 	.done(spi_cycle_done),
 	.din(data_to_send),
 	.dout(data_received)
 );

	// spi_slave_3 spi(
	// 	.clk(clk),
	// 	.rst(rst),
	// 	.ss(ss),
	// 	.mosi(mosi),
	// 	.miso(miso),
	// 	.sck(sck),
	// 	.done(spi_cycle_done),
	// 	.din(8'b11110000),
	// 	.dout(data_received)
	// );
	
    hpsfpga u0 (
        .clk_clk                            (clk),                            //                         clk.clk
        .memory_mem_a                       (HPS_DDR3_ADDR),                       //                      memory.mem_a
        .memory_mem_ba                      (HPS_DDR3_BA),                      //                            .mem_ba
        .memory_mem_ck                      (HPS_DDR3_CK_P),                      //                            .mem_ck
        .memory_mem_ck_n                    (HPS_DDR3_CK_N),                    //                            .mem_ck_n
        .memory_mem_cke                     (HPS_DDR3_CKE),                     //                            .mem_cke
        .memory_mem_cs_n                    (HPS_DDR3_CS_N),                    //                            .mem_cs_n
        .memory_mem_ras_n                   (HPS_DDR3_RAS_N),                   //                            .mem_ras_n
        .memory_mem_cas_n                   (HPS_DDR3_CAS_N),                   //                            .mem_cas_n
        .memory_mem_we_n                    (HPS_DDR3_WE_N),                    //                            .mem_we_n
        .memory_mem_reset_n                 (HPS_DDR3_RESET_N),                 //                            .mem_reset_n
        .memory_mem_dq                      (HPS_DDR3_DQ),                      //                            .mem_dq
        .memory_mem_dqs                     (HPS_DDR3_DQS_P),                     //                            .mem_dqs
        .memory_mem_dqs_n                   (HPS_DDR3_DQS_N),                   //                            .mem_dqs_n
        .memory_mem_odt                     (HPS_DDR3_ODT),                     //                            .mem_odt
        .memory_mem_dm                      (HPS_DDR3_DM),                      //                            .mem_dm
        .memory_oct_rzqin                   (HPS_DDR3_RZQ),                   //                            .oct_rzqin
        .hps_0_h2f_reset_reset_n            (not_used_hsf),            //             hps_0_h2f_reset.reset_n
        .spi_out_external_connection_export (fpga_mosi), // spi_out_external_connection.export
        .spi_in_external_connection_export  (fpga_miso),  //  spi_in_external_connection.export
        .reset_reset_n                      (1),                      //                       reset.reset_n
        .hps_io_hps_io_emac1_inst_TX_CLK    (HPS_ENET_GTX_CLK),    //                      hps_io.hps_io_emac1_inst_TX_CLK
        .hps_io_hps_io_emac1_inst_TXD0      (HPS_ENET_TX_DATA[0]),      //                            .hps_io_emac1_inst_TXD0
        .hps_io_hps_io_emac1_inst_TXD1      (HPS_ENET_TX_DATA[1]),      //                            .hps_io_emac1_inst_TXD1
        .hps_io_hps_io_emac1_inst_TXD2      (HPS_ENET_TX_DATA[2]),      //                            .hps_io_emac1_inst_TXD2
        .hps_io_hps_io_emac1_inst_TXD3      (HPS_ENET_TX_DATA[3]),      //                            .hps_io_emac1_inst_TXD3
        .hps_io_hps_io_emac1_inst_RXD0      (HPS_ENET_RX_DATA[0]),      //                            .hps_io_emac1_inst_RXD0
        .hps_io_hps_io_emac1_inst_MDIO      (HPS_ENET_MDIO),      //                            .hps_io_emac1_inst_MDIO
        .hps_io_hps_io_emac1_inst_MDC       (HPS_ENET_MDC),       //                            .hps_io_emac1_inst_MDC
        .hps_io_hps_io_emac1_inst_RX_CTL    (HPS_ENET_RX_DV),    //                            .hps_io_emac1_inst_RX_CTL
        .hps_io_hps_io_emac1_inst_TX_CTL    (HPS_ENET_TX_EN),    //                            .hps_io_emac1_inst_TX_CTL
        .hps_io_hps_io_emac1_inst_RX_CLK    (HPS_ENET_RX_CLK),    //                            .hps_io_emac1_inst_RX_CLK
        .hps_io_hps_io_emac1_inst_RXD1      (HPS_ENET_RX_DATA[1]),      //                            .hps_io_emac1_inst_RXD1
        .hps_io_hps_io_emac1_inst_RXD2      (HPS_ENET_RX_DATA[2]),      //                            .hps_io_emac1_inst_RXD2
        .hps_io_hps_io_emac1_inst_RXD3      (HPS_ENET_RX_DATA[3]),      //                            .hps_io_emac1_inst_RXD3
        .hps_io_hps_io_sdio_inst_CMD        (HPS_SD_CMD),        //                            .hps_io_sdio_inst_CMD
        .hps_io_hps_io_sdio_inst_D0         (HPS_SD_DATA[0]),         //                            .hps_io_sdio_inst_D0
        .hps_io_hps_io_sdio_inst_D1         (HPS_SD_DATA[1]),         //                            .hps_io_sdio_inst_D1
        .hps_io_hps_io_sdio_inst_CLK        (HPS_SD_CLK),        //                            .hps_io_sdio_inst_CLK
        .hps_io_hps_io_sdio_inst_D2         (HPS_SD_DATA[2]),         //                            .hps_io_sdio_inst_D2
        .hps_io_hps_io_sdio_inst_D3         (HPS_SD_DATA[3]),         //                            .hps_io_sdio_inst_D3
        .hps_io_hps_io_usb1_inst_D0         (HPS_USB_DATA[0]),         //                            .hps_io_usb1_inst_D0
        .hps_io_hps_io_usb1_inst_D1         (HPS_USB_DATA[1]),         //                            .hps_io_usb1_inst_D1
        .hps_io_hps_io_usb1_inst_D2         (HPS_USB_DATA[2]),         //                            .hps_io_usb1_inst_D2
        .hps_io_hps_io_usb1_inst_D3         (HPS_USB_DATA[3]),         //                            .hps_io_usb1_inst_D3
        .hps_io_hps_io_usb1_inst_D4         (HPS_USB_DATA[4]),         //                            .hps_io_usb1_inst_D4
        .hps_io_hps_io_usb1_inst_D5         (HPS_USB_DATA[5]),         //                            .hps_io_usb1_inst_D5
        .hps_io_hps_io_usb1_inst_D6         (HPS_USB_DATA[6]),         //                            .hps_io_usb1_inst_D6
        .hps_io_hps_io_usb1_inst_D7         (HPS_USB_DATA[7]),         //                            .hps_io_usb1_inst_D7
        .hps_io_hps_io_usb1_inst_CLK        (HPS_USB_CLKOUT),        //                            .hps_io_usb1_inst_CLK
        .hps_io_hps_io_usb1_inst_STP        (HPS_USB_STP),        //                            .hps_io_usb1_inst_STP
        .hps_io_hps_io_usb1_inst_DIR        (HPS_USB_DIR),        //                            .hps_io_usb1_inst_DIR
        .hps_io_hps_io_usb1_inst_NXT        (HPS_USB_NXT),        //                            .hps_io_usb1_inst_NXT
        .hps_io_hps_io_uart0_inst_RX        (HPS_UART_RX),        //                            .hps_io_uart0_inst_RX
        .hps_io_hps_io_uart0_inst_TX        (HPS_UART_TX),        //                            .hps_io_uart0_inst_TX
        .spi_sck_external_connection_export (fpga_sck), // spi_sck_external_connection.export
        .spi_ss_external_connection_export  (fpga_s0)   //  spi_ss_external_connection.export
    );


endmodule



// "C:\Users\Pedro\AppData\Local\Programs\Microsoft VS Code\Code.exe" -g %f:%l