import cv2
import time
from rasp_pdi import RaspPDI
from communication_controller import CommunicationController

com = None

def rasp_pdi(img):
    initial_time = time.time()
    pdi = RaspPDI()

    img = pdi.illumination_compesation(img)
    
    img_YCrCb = cv2.cvtColor(img, cv2.COLOR_BGR2YCrCb)
    Y, Cr, Cb = cv2.split(img_YCrCb)
    # img = cv2.merge([Cr, Cb, Y])
    
    img = pdi.skin_color_segmentation(Y, Cr, Cb)
    img = pdi.filtering(img)
    
    area, perimeter = pdi.hand_area_perimeter(img)
    print(f"RPi - Area: {area}, Perimeter: {perimeter}")

    reference_point = pdi.calculate_base_reference(img)
    # print(f"RPi - reference point: {reference_point}")

    pixel = pdi.find_first_edge_pixel(img)
    # print(f"RPi - first pixel: {pixel}")

    contour = pdi.find_contours(img)

    distances, contours = pdi.calculate_radial_distances(contour, reference_point)
    max_distance = max(distances)
    for i, distance in enumerate(distances):
        if distance == max_distance:
            max_index = i
            break

    # print(f"RPi - max distance: {max(distances)**2}")

    peaks = pdi.detect_peaks(distances)
    print(f"RPi - peaks: {len(peaks)}")

    pdi.check_classification(area, perimeter, peaks)

    mean_time = time.time() - initial_time
    print(f"PDI in rasp finished in: {mean_time}")
    # cv2.imshow("rpi_img", img)

def fpga_pdi(img, height, width):
    global com
    initial_time = time.time()
    com = CommunicationController(height, width)

    com.send_rgb_img(img)

    print("Image send")
    time.sleep(2)

    com.run_pdi()
    # time.sleep(2)

    new_img_r = com.recive_img(0b01)
    new_img_g = com.recive_img(0b10)
    new_img_b = com.recive_img(0b11)
    new_img = cv2.merge([new_img_b, new_img_g, new_img_r])
    
    hand_area = com.recive_int_32bits(0b00)
    hand_perimeter = com.recive_int_32bits(0b01)
    print(f"FPGA - Area: {hand_area}, Perimeter: {hand_perimeter}")
    peaks = com.recive_int_32bits(0b10)
    print(f"FPGA - peaks: {peaks}")

    classification = com.recive_int_32bits(0b11)
    # print(classification)
    if (classification == 1):
        print("FPGA Classification: One finger up")
    elif (classification == 2):
        print("FPGA Classification: Victory")
    elif (classification == 3):
        print("FPGA Classification: Three fingers up")
    elif (classification == 4):
        print("FPGA Classification: Four fingers up")
    elif (classification == 5):
        print("FPGA Classification: Open palm")
    elif (classification == 6):
        print("FPGA Classification: Closed fist")
    else:
        print("FPGA Classification: Not recognized")

    com.close_communication()

    fpga_time = time.time() - initial_time
    print(f"FPGA finished in: {fpga_time}")
    cv2.imshow("fpga_img", new_img)

def main():
    height = 240
    width = 320

    img_select = 2

    if img_select == 0: 
        img = cv2.imread('hand.jpg')
    if img_select == 1: 
        img = cv2.imread('one_finger_up.JPEG')
    if img_select == 2: 
        img = cv2.imread('victory.JPEG')
    if img_select == 3: 
        img = cv2.imread('three_fingers_up.JPEG')
    if img_select == 4: 
        img = cv2.imread('four_fingers_up.JPEG')
    if img_select == 5: 
        img = cv2.imread('open_palm.JPEG')
    if img_select == 6: 
        img = cv2.imread('closed_fist.JPEG')
    
    img = cv2.resize(img, (width, height))

    # cv2.imshow("img", img)
    # cv2.waitKey(0)
    # cv2.destroyAllWindows()

    fpga_pdi(img, height, width)

    print("\n")

    rasp_pdi(img)

    cv2.imshow("img", img)
    cv2.waitKey(0)
    cv2.destroyAllWindows()

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        com.close_communication()