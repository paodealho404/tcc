module spi_slave_3 (
    input clk,  //execution clock
    input rst,  //module reset
    input ss,  //chip select
    input mosi,  //master out slave in
    output reg miso,  //master in slave out
    input sck,  //communication clock
    output reg done,  //signal indicating transfer completed
    input [7:0] din,  //input data
    output reg [7:0] dout  //output data
);

  reg prv_sck;
  reg [7:0] bit_count;

  always @(posedge clk or negedge rst) begin
    if (!rst) begin
      dout <= 8'b0;
      miso <= 1'b0;
      done <= 1'b0;
      bit_count <= 8'b0;
    end else if (ss) begin
      dout <= 8'b0;
      miso <= din[7];
      done <= 1'b0;
      bit_count <= 8'b110;
    end else begin
      if (sck && !prv_sck) begin
        miso <= din[bit_count];
        bit_count <= bit_count - 1'b1;
        if (bit_count == 0) begin
          done <= 1'b1;
        end else begin
          done <= 1'b0;
        end
      end
    end
  end

endmodule
