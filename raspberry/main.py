import cv2
import time
from rasp_pdi import RaspPDI
from communication_controller import CommunicationController

def rasp_pdi(img):
    pdi = RaspPDI()

    img = pdi.illumination_compesation(img)

    cv2.imshow("rpi_img", img)

def fpga_pdi(img, height, width):
    com = CommunicationController(height, width)

    com.send_rgb_img(img)

    print("Image send")
    # time.sleep(2)

    com.run_pdi()
    # time.sleep(2)

    new_img_r = com.recive_img(0b01)
    new_img_g = com.recive_img(0b10)
    new_img_b = com.recive_img(0b11)
    new_img = cv2.merge([new_img_b, new_img_g, new_img_r])

    com.close_communication()

    cv2.imshow("fpga_img", new_img)

def main():
    height = 240
    width = 320
    img = cv2.imread('hand.jpg')
    img = cv2.resize(img, (width, height))

    cv2.imshow("img", img)
    cv2.waitKey(0)
    cv2.destroyAllWindows()

    fpga_pdi(img, height, width)

    rasp_pdi(img)

    cv2.waitKey(0)
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()