/*
 * Module Name: bram_image_storage.
 *
 * Description: Acts as a memory storing 76800 bytes.
 *
 * Inputs:
 *    clk - Main clock signal
 *    addr_read - Address to read from the memory
 *    addr_write - Address to write to the memory
 *    we - Write enable signal
 *    data_in - Input byte data signal
 *
 * Outputs:
 *    data_out - Output byte data signal
 *
 * Functionality:
 *    This module acts as a memory storing 76800 bytes.
 *    It has two address inputs, one for reading and one for writing. 
 *    The data_out is always provided according to addr_read.
 *    The data_in is written to the memory according to addr_write when we is high.
 *    The data_in is written to the memory addr_wire position when we is high.
 */

module bram_image_storage(
    input clk,
    input [16:0] addr_read,
    input [16:0] addr_write,
    input we,
    input [7:0] data_in,
    output reg [7:0] data_out
);

reg [7:0] bram[76799:0];

always @(negedge clk) begin
    if (we) begin
        bram[(addr_write <= 17'd76799) ? addr_write : 17'd76799] <= data_in;
    end
    data_out <= bram[(addr_read <= 17'd76799) ? addr_read : 17'd76799];
end

endmodule