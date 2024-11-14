module test_spi (
    input            clk,   // Execution clock
    input            rst,   // Module reset
    input            ss,    // Chip select
    input            mosi,  // Master Out Slave In
    output reg       miso,  // Master In Slave Out
    input            sck,   // Communication clock
    output reg       done,  // Signal indicating transfer completed
    input      [7:0] din,   // Data input
    output reg [7:0] dout,  // Data output
    output reg [2:0] count
);

  reg [7:0] buffer;  // Buffer to hold incoming bits
  //reg [2:0] count;        // Count bits received
  reg       prev_sck;  // Previous state of sck to detect edges

  //	 always @ (sck or rst) begin
  //		
  //		if ((count == 3'b111) || (!rst)) begin
  //			count = 3'b000;
  //		end
  //		else begin
  //			if (sck) begin
  //				count = count + 3'b001;
  //			end
  //		end
  //	 end

  always @(posedge clk or negedge rst) begin
    if (!rst) begin
      dout <= 8'b0;
      buffer <= 8'b0;
      count <= 4'b000;
      done <= 1'b0;
      prev_sck <= 1'b0;
    end else begin
      if (!prev_sck && sck) begin
        if (ss == 1'b0) begin
          buffer <= {buffer[6:0], mosi};
          if (count == 3'b111) begin
            dout  <= {buffer[6:0], mosi};
            done  <= 1'b1;
            count <= 3'b000;
          end else begin
            count <= count + 1'b1;
            done  <= 1'b0;
          end
        end
      end else if (ss == 1'b1) begin
        count <= 3'b000;
        done  <= 1'b0;
      end
      prev_sck <= sck;
    end
  end
endmodule
