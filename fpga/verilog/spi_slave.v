/*
 * Module Name: spi_slave.
 *
 * Description: Executes SPI communication.
 *
 * Inputs:
 *    clk - Main clock signal
 *    rst - Reset signal
 *    ss - Chip select signal
 *    mosi - Master out slave in signal
 *    sck - Communication clock signal
 *    din - Input byte data to be sent
 *
 * Outputs:
 *    miso - Master in slave out signal
 *    done - Signal that indicates when a SPI cycle is done (receiving or sending a byte)
 *    dout - Output byte data received
 *
 * Functionality:
 *    Performs SPI communication.
 *    When selected by the master (ss), it sends and receives a bit every sck cycle.
 *    When one byte is received and sent, the done signal is set.
 *    Source: https://alchitry.com/serial-peripheral-interface-spi-verilog (removed from the website?)
 */

module spi_slave (
    input clk,  //execution clock
    input rst,  //module reset
    input ss,  //chip select
    input mosi,  //master out slave in
    output miso,  //master in slave out
    input sck,  //communication clock
    output done,  //signal indicating transfer completed
    input [7:0] din,  //input data
    output [7:0] dout  //output data
);

  reg mosi_d, mosi_q;
  reg ss_d, ss_q;
  reg sck_d, sck_q;
  reg sck_old_d, sck_old_q;
  reg [7:0] data_d, data_q;
  reg done_d, done_q;
  reg [2:0] bit_ct_d, bit_ct_q;
  reg [7:0] dout_d, dout_q;
  reg miso_d, miso_q;

  assign miso = miso_q;
  assign done = done_q;
  assign dout = dout_q;

  always @(*) begin
    ss_d = ss;
    mosi_d = mosi;
    miso_d = miso_q;
    sck_d = sck;
    sck_old_d = sck_q;
    data_d = data_q;
    done_d = 1'b0;
    bit_ct_d = bit_ct_q;
    dout_d = dout_q;

    if (ss_q) begin  // if slave select is high (deselcted)
      bit_ct_d = 3'b0;  // reset bit counter
      data_d   = din;  // read in data
      miso_d   = data_q[7];  // output MSB
    end else begin  // else slave select is low (selected)
      if (!sck_old_q && sck_q) begin  // rising edge
        data_d   = {data_q[6:0], mosi_q};  // read data in and shift
        bit_ct_d = bit_ct_q + 1'b1;  // increment the bit counter
        if (bit_ct_q == 3'b111) begin  // if we are on the last bit
          dout_d = {data_q[6:0], mosi_q};  // output the byte
          done_d = 1'b1;  // set transfer done flag
          data_d = din;  // read in new byte
        end
      end else if (sck_old_q && !sck_q) begin  // falling edge
        miso_d = data_q[7];  // output MSB
      end
    end
  end

  always @(posedge clk) begin
    if (!rst) begin
      done_q   <= 1'b0;
      bit_ct_q <= 3'b0;
      dout_q   <= 8'b0;
      miso_q   <= 1'b1;
    end else begin
      done_q   <= done_d;
      bit_ct_q <= bit_ct_d;
      dout_q   <= dout_d;
      miso_q   <= miso_d;
    end

    sck_q <= sck_d;
    mosi_q <= mosi_d;
    ss_q <= ss_d;
    data_q <= data_d;
    sck_old_q <= sck_old_d;

  end

endmodule
