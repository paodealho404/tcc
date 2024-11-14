import cv2
import time
import numpy as np
import spidev

class CommunicationController:

    def __init__(self, height: int, width: int) -> None:
        self.spi = spidev.SpiDev()
        self.spi.open(0, 0)
        self.spi.max_speed_hz = 37000000
        self.spi.mode = 0

        self.height = height
        self.width = width

    def sendbyte(self, byte_to_send: list[int]) -> list[int]:
        # time.sleep(self.delay_time)
        # received = self.spi.exange_data(byte_to_send)
        received = self.spi.xfer3(byte_to_send)
        # print("Byte enviado:  {:08b}".format(byte_to_send), "Byte recebido: {:08b}".format(received))
        return received

    def send_img(self, img: np.ndarray, channel: int = 0b10) -> np.ndarray:
        initial_time = time.time()

        height, width = img.shape[0:2]
        height_bytes = self.toUnint8(height, 2)
        width_bytes = self.toUnint8(width, 2)

        self.spi.xfer([0, int(0b00000100 | channel),
                        int(height_bytes[0]), int(height_bytes[1]),
                        int(width_bytes[0]), int(width_bytes[1])])

        array_pixels = img.flatten().astype(np.int32).tolist()
        self.spi.xfer3(array_pixels)

        self.spi.xfer([0])

        send_time = time.time() - initial_time
        print(f"Time to send image: {send_time}")

    def recive_img(self, channel: int = 0b10) -> np.array:
        self.spi.xfer([0, int(0b00001000 | channel), 0, 0])

        pixels_array = []

        for i in range(76800):
            result = self.spi.xfer([0])
            pixels_array.append(result)
        
        pixels_array = np.array(pixels_array, dtype=np.uint8)
        new_img = pixels_array.reshape(240, 320)

        self.spi.xfer([0])
        return new_img
    
    def send_rgb_img(self, img: np.ndarray) -> None:
        initial_time = time.time()

        channel_b, channel_g, channel_r = cv2.split(img)

        print("Sending red")
        self.send_img(channel_r, 0b01)
        print("Sending green")
        self.send_img(channel_g, 0b10)
        print("Sending blue")
        self.send_img(channel_b, 0b11)

        send_time = time.time() - initial_time
        print(f"All channels sended in: {send_time}")

    def run_pdi(self) -> None:

        initial_time = time.time()

        self.spi.xfer([0, int(0b00001100), 0, 0])

        print("FPGA on PDI")
        pdi_running = False
        while(not pdi_running):
            print(".", end='')
            pdi_running = self.spi.xfer([0])[0]
        while(pdi_running):
            print(".", end='')
            pdi_running = self.spi.xfer([0])[0]
        print(".")
        
        self.spi.xfer([0])

        pdi_time = time.time() - initial_time
        print(f"PDI in FPGA finished in: {pdi_time}")

    def toUnint8(self, data: int, num_bytes: int) -> np.array:
        data_bytes = data.to_bytes(num_bytes, "big")
        return np.frombuffer(data_bytes, dtype=np.uint8)

    def close_communication(self) -> None:
        print("Closing connection")
        self.spi.close()

        

