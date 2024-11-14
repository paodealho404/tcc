library verilog;
use verilog.vl_types.all;
entity data_transfer_controller is
    port(
        clk             : in     vl_logic;
        rst             : in     vl_logic;
        spi_cycle_done  : in     vl_logic;
        spi_byte_in     : in     vl_logic_vector(7 downto 0);
        spi_byte_out    : out    vl_logic_vector(7 downto 0);
        bram_addr       : out    vl_logic_vector(16 downto 0);
        bram_channel    : out    vl_logic_vector(1 downto 0);
        bram_we         : out    vl_logic;
        bram_data_in    : out    vl_logic_vector(7 downto 0);
        bram_data_out   : in     vl_logic_vector(7 downto 0);
        pdi_active      : out    vl_logic;
        pdi_done        : in     vl_logic;
        size_byte_count : out    vl_logic_vector(2 downto 0)
    );
end data_transfer_controller;
