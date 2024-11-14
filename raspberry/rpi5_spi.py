import gpiod
import lgpio
import time
import numpy as np

class RPi5SPI:

    __MOSI_PIN = 10
    __MISO_PIN = 9
    __SCK_PIN = 11
    __SS_PIN = 22
    __CHIP_NAME = "gpiochip4"

    __SCK_PERIOD = 1
    __HALF_PERIOD = __SCK_PERIOD/2
    __QUARTER_PERIOD = __SCK_PERIOD/4

    def __init__(self) -> None:
        # self.chip = gpiod.Chip(self.__CHIP_NAME)

        # self.__line_mosi = self.chip.get_line(self.__MOSI_PIN)
        # self.__line_miso = self.chip.get_line(self.__MISO_PIN)
        # self.__line_sck = self.chip.get_line(self.__SCK_PIN)
        # self.__line_ss = self.chip.get_line(self.__SS_PIN)

        # self.__line_mosi.request(consumer="spi_mosi", type=gpiod.LINE_REQ_DIR_OUT)
        # self.__line_miso.request(consumer="spi_miso", type=gpiod.LINE_REQ_DIR_IN)
        # self.__line_sck.request(consumer="spi_sck", type=gpiod.LINE_REQ_DIR_OUT)
        # self.__line_ss.request(consumer="spi_ss", type=gpiod.LINE_REQ_DIR_OUT)

        self.handler = lgpio.gpiochip_open(4)

        lgpio.gpio_claim_output(self.handler, self.__MOSI_PIN)
        lgpio.gpio_claim_input(self.handler, self.__MISO_PIN)
        lgpio.gpio_claim_output(self.handler, self.__SCK_PIN)
        lgpio.gpio_claim_output(self.handler, self.__SCK_PIN)
        
        lgpio.gpio_write(self.handler, self.__SCK_PIN, 0)
        lgpio.gpio_write(self.handler, self.__SS_PIN, 1)

    def close_connection(self) -> None:
        # self.__line_mosi.release()
        # self.__line_miso.release()
        # self.__line_sck.release()
        # self.__line_ss.release()
        lgpio.gpiochip_close(self.handler)

    def exange_data(self, byte_out : np.uint8) -> int:
        byte_in = 0
        
        # self.__line_ss.set_value(0)
        
        # for i in range(8):
        #     self.__line_mosi.set_value((byte_out >> (7 - i)) & 0x01)
        #     # time.sleep(self.__QUARTER_PERIOD)
        #     self.__line_sck.set_value(1)
        #     # time.sleep(self.__QUARTER_PERIOD)
        #     bit = self.__line_miso.get_value()
        #     # print(bit)
        #     byte_in = (byte_in << 1) | bit
        #     self.__line_sck.set_value(0)
        #     # time.sleep(self.__HALF_PERIOD)
        
        # self.__line_ss.set_value(1)

        lgpio.gpio_write(self.handler, self.__SS_PIN, 0)

        for i in range(0):
            lgpio.gpio_write(self.handler, self.__MOSI_PIN, ((byte_out >> (7 - i)) & 0x01))
            time.sleep(self.__QUARTER_PERIOD)
            lgpio.gpio_write(self.handler, self.__SCK_PIN, 1)
            time.sleep(self.__QUARTER_PERIOD)
            bit = lgpio.gpio_read(self.handler, self.__MISO_PIN)
            byte_in = (byte_in << 1) | bit
            lgpio.gpio_write(self.handler, self.__SCK_PIN, 0)
            time.sleep(self.__HALF_PERIOD)

        lgpio.gpio_write(self.handler, self.__SS_PIN, 1)

        return byte_in

    def set_period(self, time) -> None:
        self.__SCK_PERIOD = time
        self.__HALF_PERIOD = self.__SCK_PERIOD/2
        self.__QUARTER_PERIOD = self.__SCK_PERIOD/4

    def set_frequency(self, frequency) -> None:
        self.__SCK_PERIOD = (1/frequency)
        self.__HALF_PERIOD = self.__SCK_PERIOD/2
        self.__QUARTER_PERIOD = self.__SCK_PERIOD/4