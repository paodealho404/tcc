/*
 * Module Name: img_processing.
 *
 * Description: Executes PDI.
 *
 * Inputs:
 *    clk - Main clock signal
 *    rst - Reset signal
 *    active - Signal that activates PDI execution
 *    red_data_in - Input byte data for red channel
 *    green_data_in - Input byte data for green channel
 *    blue_data_in - Input byte data for blue channel
 *
 * Outputs:
 *    done - Signal that indicates when a PDI cycle is done
 *    red_data_out - Output byte data for red channel
 *    green_data_out - Output byte data for green channel
 *    blue_data_out - Output byte data for blue channel
 *    we - BRAM write enable signal
 *    addr_read - Address for reading memory
 *    addr_write - Address for writing memory
 *    mean - Debug signal
 *
 * Functionality:
 *    State machine that processes PDI.
 *    States:
 *      - 000: Initializes values and waits for active signal
 *      - 001: Accumulates the data for the mean calculation
 *      - 010: Calculates the mean for each channel
 *      - 011: Calculates the max mean and prepares the data for the next state
 *      - 100: Executes the ilumination compesation
 */

module img_processing (
    input clk,
    input rst,
    input active,
    output reg done,

    input [7:0] red_data_in,
    input [7:0] green_data_in,
    input [7:0] blue_data_in,

    output reg [7:0] red_data_out,
    output reg [7:0] green_data_out,
    output reg [7:0] blue_data_out,

    output reg we,
    output reg [16:0] addr_read,
    output reg [16:0] addr_write,
    output reg [16:0] hand_area,
    output reg [16:0] hand_perimeter,
    output reg [34:0] max_distance,
    output reg [9:0] peaks,
    output reg [3:0] classification
);

  reg [3:0] state;

  reg [24:0] red_acumulator;
  reg [24:0] green_acumulator;
  reg [24:0] blue_acumulator;

  reg [7:0] red_mean;
  reg [7:0] green_mean;
  reg [7:0] blue_mean;
  reg [7:0] max_mean;

  reg [15:0] temp_red;
  reg [15:0] temp_green;
  reg [15:0] temp_blue;

  reg [16:0] morphology_index_collumn;
  reg [16:0] morphology_index_row;
  reg [2:0] aux_index;
  reg [4:0] morphology_kernel;

  // reg [16:0] hand_area;
  // reg [16:0] hand_perimeter;

  reg [7:0] previous_pixel;
  reg [16:0] reference_x;
  reg [16:0] init_x;
  // reg [16:0] init_y;
  reg [16:0] current_x;
  reg [16:0] current_y;

  reg [34:0] distance_buffer[899:0];
  // reg [34:0] max_distance;
  reg [9:0] distance_buffer_index;
  reg [9:0] max_distance_index;

  reg signed [3:0] directions[0:7][0:1];
  reg [2:0] current_direction;
  reg [3:0] direction_index;
  reg [3:0] neighbor_index;

  reg [16:0] first_edge;
  reg [16:0] prev_edge;
  reg [16:0] edge_candidate;

  reg [9:0] prev_index;
  reg [34:0] prev_distance;
  reg [34:0] prev_prev_distance;
  // reg [9:0] peaks;

  wire [16:0] dx;
  wire [16:0] dy;
  wire [34:0] distance_squared;

  wire signed [18:0] edge_offset;
  wire signed [18:0] edge_candidate_calc;
  wire signed [18:0] neighbor_offset;
  wire signed [18:0] neighbor_calc;

  assign edge_offset = directions[(current_direction + direction_index) % 8][0] == 2 ?  prev_edge - 17'd320 : prev_edge + (directions[(current_direction + direction_index) % 8][0] * 320);
  assign edge_candidate_calc = directions[(current_direction + direction_index) % 8][1] == 2 ? edge_offset - 1 : edge_offset + directions[(current_direction + direction_index) % 8][1];
  assign neighbor_offset = directions[neighbor_index][0] == 2 ?  edge_candidate - 17'd320 : edge_candidate + (directions[neighbor_index][0] * 320);
  assign neighbor_calc = directions[neighbor_index][1] == 2 ? neighbor_offset - 1 : neighbor_offset + directions[neighbor_index][1];

  assign dx = (current_x > reference_x) ? (current_x - reference_x) : (reference_x - current_x);
  assign dy = (current_y > 17'd239) ? (current_y - 17'd239) : (17'd239 - current_y);
  assign distance_squared = (dx * dx) + (dy * dy);

  task init_values;
    begin
      state <= 4'd0;
      done <= 1'b0;
      we <= 1'b0;
      addr_read <= 17'b0;
      addr_write <= {17{1'b1}};
      red_acumulator <= 25'b0;
      green_acumulator <= 25'b0;
      blue_acumulator <= 25'b0;
      red_data_out <= 8'b0;
      green_data_out <= 8'b0;
      blue_data_out <= 8'b0;
      temp_blue <= 16'b0;
      temp_green <= 16'b0;
      temp_red <= 16'b0;
      morphology_index_collumn <= 17'b0;
      morphology_index_row <= 17'b0;
      aux_index <= 3'b0;
      morphology_kernel <= 5'b0;
      previous_pixel <= 8'b0;
      reference_x <= 17'b0;
      init_x <= 17'b0;
      // init_y <= 17'b0;
      current_x <= 17'b0;
      current_y <= 17'b0;
      // max_distance <= 35'b0;
      distance_buffer_index <= 10'b0;
      max_distance_index <= 10'b0;
      current_direction <= 3'b0;
      direction_index <= 4'b0;
      neighbor_index <= 4'b0;
      first_edge <= 17'b0;
      edge_candidate <= 17'b0;
      prev_index <= 10'b0;
      prev_distance <= 35'b0;
      prev_prev_distance <= 35'b0;
      // peaks <= 10'b0;

      directions[0][0] = 2;
      directions[0][1] = 2;
      directions[1][0] = 2;
      directions[1][1] = 0;
      directions[2][0] = 2;
      directions[2][1] = 1;
      directions[3][0] = 0;
      directions[3][1] = 1;
      directions[4][0] = 1;
      directions[4][1] = 1;
      directions[5][0] = 1;
      directions[5][1] = 0;
      directions[6][0] = 1;
      directions[6][1] = 2;
      directions[7][0] = 0;
      directions[7][1] = 2;
    end
  endtask

  always @(posedge clk) begin
    if (!rst) begin
      init_values;
    end else begin
      case (state)
        4'd0: begin  // Wait for active signal
          if (active && !done) begin
            state <= 4'd1;
          end else if (done) begin
            if (!active) begin
              done <= 1'b0;  // Reset done signal when active signal is low
            end
          end else begin
            init_values;
          end
        end
        4'd1: begin  // Accumulate data for the mean calculation
          hand_area <= 17'd0;
          hand_perimeter <= 17'd0;
          max_distance <= 35'd0;
          peaks <= 10'd0;
          classification <= 4'd0;

          addr_read <= addr_read + 1'b1;
          red_acumulator <= red_acumulator + red_data_in;
          green_acumulator <= green_acumulator + green_data_in;
          blue_acumulator <= blue_acumulator + blue_data_in;
          red_mean <= red_acumulator[7:0];
          if (addr_read >= 17'd76799) begin
            state <= 4'd2;
          end
        end
        4'd2: begin  // Calculate the mean for each channel
          red_mean <= (red_acumulator / 17'd76800);
          green_mean <= (green_acumulator / 17'd76800);
          blue_mean <= (blue_acumulator / 17'd76800);

          addr_read <= 17'd76799;
          state <= 4'd3;
        end
        4'd3: begin  // Calculate the max mean
          if (red_mean > green_mean && red_mean > blue_mean) begin
            max_mean <= red_mean;
          end else if (green_mean > red_mean && green_mean > blue_mean) begin
            max_mean <= green_mean;
          end else begin
            max_mean <= blue_mean;
          end

          // pre-calculate the 76799 pixel value
          temp_red <= red_data_in * red_mean;
          temp_green <= green_data_in * green_mean;
          temp_blue <= blue_data_in * blue_mean;

          addr_read <= 17'd0;
          addr_write <= {17{1'b1}};
          state <= 4'd4;
        end
        4'd4: begin  // Execute the ilumination compesation
          //todo: need time improvement
          we <= 1'b1;
          addr_read <= addr_read + 1'b1;
          addr_write <= addr_read - 1'b1;

          // Multiply current interation pixel by the channel mean
          temp_red <= red_data_in * red_mean;
          temp_green <= green_data_in * green_mean;
          temp_blue <= blue_data_in * blue_mean;

          // Divide previous interation result by the max mean
          red_data_out <= temp_red / max_mean;
          green_data_out <= temp_green / max_mean;
          blue_data_out <= temp_blue / max_mean;

          if (addr_read >= 17'd76799) begin
            state <= 4'd5;
            addr_read <= 17'd0;
            addr_write <= 17'd0;
          end
        end
        4'd5: begin  // Convert RGb to YCbCr
          we <= 1'b1;
          addr_read <= addr_read + 1'b1;
          addr_write <= addr_read;

          // Y
          // red_data_out <= 16 + ((
          //         (red_data_in<<6) + (red_data_in<<1) +
          //         (green_data_in<<7) + green_data_in +
          //         (blue_data_in<<4) + (blue_data_in<<3) + blue_data_in
          //     )>>8);

          // Cb
          green_data_out <= 128 + ((
                            -((red_data_in<<5) + (red_data_in<<2) + (red_data_in<<1)) -
                            ((green_data_in<<6) + (green_data_in<<3) + (green_data_in<<1)) +
                            (blue_data_in<<7) - (blue_data_in<<4)
                        )>>8);

          // Cr
          blue_data_out <= 128 + ((
                            (red_data_in<<7) - (red_data_in<<4) -
                            ((green_data_in<<6) + (green_data_in<<5) - (green_data_in<<1)) -
                            ((blue_data_in<<4) + (blue_data_in<<1))
                        )>>8);

          if (addr_read >= 17'd76799) begin
            state <= 4'd6;
            addr_read <= 17'd0;
            addr_write <= 17'd0;
          end
        end
        4'd6: begin  // binarization
          we <= 1'b1;
          addr_read <= addr_read + 1'b1;
          addr_write <= addr_read;

          if (green_data_in >= 90 && green_data_in <= 120 && blue_data_in >= 139 && blue_data_in <= 170) begin
            red_data_out   <= 255;
            green_data_out <= 255;
            blue_data_out  <= 255;
          end else begin
            red_data_out   <= 0;
            green_data_out <= 0;
            blue_data_out  <= 0;
          end

          if (addr_read >= 17'd76799) begin
            state <= 4'd7;
            morphology_index_collumn <= 17'd1;
            morphology_index_row <= 17'd320;
            aux_index <= 3'b001;
            addr_read <= 17'd1;
            addr_write <= 17'd321;
            we <= 1'b0;
          end
        end
        4'd7: begin  // Erosion
          if (aux_index >= 3'b110) begin
            aux_index <= 3'b0;
          end else begin
            aux_index <= aux_index + 1'b1;
          end

          if (aux_index == 3'b000) begin
            we <= 1'b0;
            addr_read <= morphology_index_collumn + morphology_index_row - 17'd320;
          end else if (aux_index == 3'b001) begin
            we <= 1'b0;
            addr_read <= morphology_index_collumn + morphology_index_row - 17'd1;
            morphology_kernel[0] <= red_data_in[0];
          end else if (aux_index == 3'b010) begin
            we <= 1'b0;
            addr_read <= morphology_index_collumn + morphology_index_row;
            morphology_kernel[1] <= red_data_in[0];
          end else if (aux_index == 3'b011) begin
            we <= 1'b0;
            addr_read <= morphology_index_collumn + morphology_index_row + 17'd1;
            morphology_kernel[2] <= red_data_in[0];
          end else if (aux_index == 3'b100) begin
            we <= 1'b0;
            addr_read <= morphology_index_collumn + morphology_index_row + 17'd320;
            morphology_kernel[3] <= red_data_in[0];
          end else if (aux_index == 3'b101) begin
            we <= 1'b0;
            morphology_kernel[4] <= red_data_in[0];
          end else if (aux_index == 3'b110) begin
            we <= 1'b1;
            addr_write <= morphology_index_collumn + morphology_index_row;

            // Erosion
            if (morphology_kernel[0] & morphology_kernel[1] & morphology_kernel[2] & morphology_kernel[3] & morphology_kernel[4]) begin
              red_data_out   <= 8'd255;
              blue_data_out  <= 8'd255;
              green_data_out <= 8'd255;
            end else if (morphology_kernel[2]) begin
              red_data_out   <= 8'd1;
              blue_data_out  <= 8'd1;
              green_data_out <= 8'd1;
            end else begin
              red_data_out   <= 8'd0;
              blue_data_out  <= 8'd0;
              green_data_out <= 8'd0;
            end

            if (morphology_index_collumn >= 17'd318) begin
              morphology_index_collumn <= 17'd1;
              morphology_index_row <= morphology_index_row + 17'd320;
            end else begin
              morphology_index_collumn <= morphology_index_collumn + 17'd1;
            end
          end

          if (addr_read >= 17'd76799) begin
            state <= 4'd8;
            addr_read <= 17'd0;
            addr_write <= 17'd0;
          end
        end
        4'd8: begin  // Erosion finalization
          we <= 1'b1;
          addr_read <= addr_read + 1'b1;
          addr_write <= addr_read;

          if (red_data_in < 8'd255) begin
            red_data_out   <= 8'd0;
            green_data_out <= 8'd0;
            blue_data_out  <= 8'd0;
          end else begin
            red_data_out   <= 8'd255;
            green_data_out <= 8'd255;
            blue_data_out  <= 8'd255;
          end

          if (addr_read >= 17'd76799) begin
            state <= 4'd9;
            morphology_index_collumn <= 17'd1;
            morphology_index_row <= 17'd320;
            aux_index <= 3'b001;
            addr_read <= 17'd1;
            addr_write <= 17'd321;
            we <= 1'b0;
          end
        end
        4'd9: begin  // Dilate
          if (aux_index >= 3'b110) begin
            aux_index <= 3'b0;
          end else begin
            aux_index <= aux_index + 1'b1;
          end

          if (aux_index == 3'b000) begin
            we <= 1'b0;
            addr_read <= morphology_index_collumn + morphology_index_row - 17'd320;
          end else if (aux_index == 3'b001) begin
            we <= 1'b0;
            addr_read <= morphology_index_collumn + morphology_index_row - 17'd1;
            morphology_kernel[0] <= red_data_in[0];
          end else if (aux_index == 3'b010) begin
            we <= 1'b0;
            addr_read <= morphology_index_collumn + morphology_index_row;
            morphology_kernel[1] <= red_data_in[0];
          end else if (aux_index == 3'b011) begin
            we <= 1'b0;
            addr_read <= morphology_index_collumn + morphology_index_row + 17'd1;
            morphology_kernel[2] <= red_data_in[0];
          end else if (aux_index == 3'b100) begin
            we <= 1'b0;
            addr_read <= morphology_index_collumn + morphology_index_row + 17'd320;
            morphology_kernel[3] <= red_data_in[0];
          end else if (aux_index == 3'b101) begin
            we <= 1'b0;
            morphology_kernel[4] <= red_data_in[0];
          end else if (aux_index == 3'b110) begin
            we <= 1'b1;
            addr_write <= morphology_index_collumn + morphology_index_row;

            // Dilation
            if (morphology_kernel[0] | morphology_kernel[1] | morphology_kernel[2] | morphology_kernel[3] | morphology_kernel[4]) begin
              if (morphology_kernel[2]) begin
                red_data_out   <= 8'd255;
                blue_data_out  <= 8'd255;
                green_data_out <= 8'd255;
              end else begin
                red_data_out   <= 8'd2;
                blue_data_out  <= 8'd2;
                green_data_out <= 8'd2;
              end
            end else begin
              red_data_out   <= 8'd0;
              blue_data_out  <= 8'd0;
              green_data_out <= 8'd0;
            end

            if (morphology_index_collumn >= 17'd318) begin
              morphology_index_collumn <= 17'd1;
              morphology_index_row <= morphology_index_row + 17'd320;
            end else begin
              morphology_index_collumn <= morphology_index_collumn + 17'd1;
            end
          end

          if (addr_read >= 17'd76799) begin
            state <= 4'd10;
            addr_read <= 17'd0;
            addr_write <= 17'd0;
          end
        end
        4'd10: begin  // dilate finalization
          we <= 1'b1;
          addr_read <= addr_read + 1'b1;
          addr_write <= addr_read;

          if (red_data_in > 8'd0) begin
            red_data_out   <= 8'd255;
            green_data_out <= 8'd255;
            blue_data_out  <= 8'd255;
          end else begin
            red_data_out   <= 8'd0;
            green_data_out <= 8'd0;
            blue_data_out  <= 8'd0;
          end

          if (addr_read >= 17'd76799) begin
            state <= 4'd11;
            addr_read <= 17'd0;
          end
        end
        4'd11: begin  // Calculate hand area and perimeter, find refecence point and init point
          we <= 1'b0;
          addr_read <= addr_read + 1'b1;

          if (red_data_in > 8'b0) begin
            hand_area <= hand_area + 17'd1;
          end

          if (red_data_in != previous_pixel) begin
            hand_perimeter <= hand_perimeter + 17'd1;

            if (addr_read >= 17'd76480) begin
              if (init_x == 17'd0 && red_data_in == 8'd255) begin
                init_x <= (addr_read >> 1);
              end else if (init_x != 17'd0 && red_data_in == 8'd0) begin
                reference_x <= ((addr_read >> 1) + init_x) - 17'd76480;
              end
            end
          end

          previous_pixel <= red_data_in;

          if (addr_read >= 17'd76799) begin
            // init_x <= (init_x<<1) - 17'd76479;
            // init_y <= 17'd239;
            current_x <= (init_x << 1) - 17'd76479;
            current_y <= 17'd239;
            addr_read <= init_x << 1;
            first_edge <= init_x << 1;
            edge_candidate <= init_x << 1;
            aux_index <= 3'b000;
            distance_buffer_index <= 10'd0;
            state <= 4'd12;
          end
        end
        4'd12: begin  // Find distance between reference point and edge pixels
          if (aux_index == 3'b000) begin // Calculate distance between reference point and edge pixels
            distance_buffer[distance_buffer_index] <= distance_squared;
            distance_buffer_index <= distance_buffer_index + 1'd1;
            prev_edge <= edge_candidate;
            direction_index <= 3'd0;
            aux_index <= 3'b001;
            if (distance_squared > max_distance) begin
              max_distance <= distance_squared;
            end
            if (distance_buffer_index == 10'd0) begin
              prev_prev_distance <= distance_squared;
            end
            if (distance_buffer_index == 10'd1) begin
              prev_distance <= distance_squared;
            end
            if (distance_buffer_index >= 10'd899) begin
              state <= 4'd13;
            end
          end
          if (aux_index == 3'b001) begin  // Find next pixel
            addr_read <= edge_candidate_calc;
            edge_candidate <= edge_candidate_calc;
            if (direction_index > 3'd7) begin  // No edge pixel found
              state <= 4'd13;
            end
            neighbor_index <= 4'b000;
            aux_index <= 3'b010;
          end
          if (aux_index == 3'b010) begin  // Check if next pixel is hand pixel
            if (red_data_in > 8'd0) begin  // Is hand pixel
              aux_index <= 3'b011;
              neighbor_index <= 3'b001;
              addr_read <= neighbor_calc;
            end else begin  // Search for next edge candidate
              direction_index <= direction_index + 1'b1;
              aux_index <= 3'b001;
            end
          end
          if (aux_index == 3'b011) begin  // Check if next pixel in edge pixel
            if (red_data_in == 8'd0) begin  // Edge pixel
              current_direction <= (current_direction + direction_index + 6) % 8;
              current_x <= edge_candidate % 320;
              current_y <= edge_candidate / 320;
              if ((edge_candidate == first_edge) || ((edge_candidate / 320) >= 17'd239)) begin
                state <= 4'd13;
              end else begin
                aux_index <= 3'b000;
              end
            end else begin  // Search for next edge candidate neighbor
              addr_read <= neighbor_calc;
              neighbor_index <= neighbor_index + 1'b1;
              if (neighbor_index == 4'b1000) begin  // Is not edge pixel
                direction_index <= direction_index + 1'b1;
                aux_index <= 3'b001;
              end
            end
          end
        end
        4'd13: begin  // Calculate threshold
          max_distance <= (max_distance * 510) / 1000;
          max_distance_index <= distance_buffer_index;
          distance_buffer_index <= 10'd2;
          state <= 4'd14;
        end
        4'd14: begin  // Find peaks

          if ((prev_prev_distance <= prev_distance) && (prev_distance >= distance_buffer[distance_buffer_index])) begin
            if (prev_distance >= max_distance) begin
              if ((distance_buffer_index - 1 - prev_index) > 10) begin
                peaks <= peaks + 1'b1;
                prev_index <= distance_buffer_index - 1'b1;
              end
            end
          end

          prev_distance <= distance_buffer[distance_buffer_index];
          prev_prev_distance <= prev_distance;
          distance_buffer_index <= distance_buffer_index + 1'b1;

          if (distance_buffer_index >= max_distance_index) begin
            state <= 4'd15;
            // state <= 4'd0;
            // done <= 1'b1;
          end
        end
        4'd15: begin  // Classify hand
          // if ((hand_area > 17'd14949) && (hand_area < 17'd18537) && (hand_perimeter > 17'd440) && (hand_perimeter < 17'd650) && (peaks == 10'd1)) begin
          //     classification <= 4'd1;
          // end
          // else if ((hand_area > 17'd14949) && (hand_area < 17'd18537) && (hand_perimeter > 17'd440) && (hand_perimeter < 17'd650) && (peaks == 10'd2)) begin
          //     classification <= 4'd2;
          // end
          // else if ((hand_area > 17'd14949) && (hand_area < 17'd18936) && (hand_perimeter > 17'd440) && (hand_perimeter < 17'd650) && (peaks == 10'd3)) begin
          //     classification <= 4'd3;
          // end
          // else if ((hand_area > 17'd15946) && (hand_perimeter > 17'd610) && (peaks == 10'd4)) begin
          //     classification <= 4'd4;
          // end
          // else if ((hand_area >= 17'd18936) && (hand_perimeter >= 17'd687) && (peaks == 10'd5)) begin
          //     classification <= 4'd5;
          // end
          // else if ((hand_area < 17'd9966) && (hand_perimeter < 17'd381)) begin
          //     classification <= 4'd6;
          // end
          // else begin
          //     classification <= 4'd7;
          // end

          if ((hand_perimeter > 17'd440) && (hand_perimeter < 17'd660) && (peaks == 10'd1)) begin
            classification <= 4'd1;
          end
                    else if ((hand_perimeter > 17'd440) && (hand_perimeter < 17'd660) && (peaks == 10'd2)) begin
            classification <= 4'd2;
          end
                    else if ((hand_perimeter > 17'd440) && (hand_perimeter < 17'd660) && (peaks == 10'd3)) begin
            classification <= 4'd3;
          end else if ((hand_perimeter > 17'd610) && (peaks == 10'd4)) begin
            classification <= 4'd4;
          end else if ((hand_perimeter >= 17'd687) && (peaks == 10'd5)) begin
            classification <= 4'd5;
          end else if ((hand_perimeter < 17'd381)) begin
            classification <= 4'd6;
          end else begin
            classification <= 4'd7;
          end

          state <= 4'd0;
          done  <= 1'b1;
        end
        default: begin
          init_values;
        end
      endcase
    end
  end

endmodule
