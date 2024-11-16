import cv2
import time
import numpy as np

class RaspPDI:

    def __init__(self) -> None:
        pass

    def illumination_compesation (self, img):
        height, width = img.shape[0:2]
        channel_b, channel_g, channel_r = cv2.split(img)

        mean_r = cv2.mean(channel_r)[0]
        # print("R mean:", mean_r)
        mean_g = cv2.mean(channel_g)[0]
        # print("G mean:", mean_g)
        mean_b = cv2.mean(channel_b)[0]
        # print("B mean:", mean_b)

        max_mean = mean_r
        if (mean_g > max_mean):
            max_mean = mean_g
        if (mean_b > max_mean):
            max_mean = mean_b

        red_gain = mean_r/max_mean
        green_gain = mean_g/max_mean
        blue_gain = mean_b/max_mean

        for i in range(height):
            for j in range(width):
                channel_r[i,j] = channel_r[i,j]*red_gain
                channel_g[i,j] = channel_g[i,j]*green_gain
                channel_b[i,j] = channel_b[i,j]*blue_gain

        pdi_img = cv2.merge([channel_b, channel_g, channel_r])

        return pdi_img
    
    def skin_color_segmentation(self, Y, Cr, Cb):
        cb_min, cb_max = 95, 120
        cr_min, cr_max = 140, 170

        mask_cb = cv2.inRange(Cb, cb_min, cb_max)
        mask_cr = cv2.inRange(Cr, cr_min, cr_max)

        mask = cv2.bitwise_and(mask_cb, mask_cr)

        return mask
    
    def filtering(self, img):
        kernel = np.array([
            [0, 1, 0],
            [1, 1, 1],
            [0, 1, 0]
        ], dtype=np.uint8)

        eroded_img = cv2.erode(img, kernel, iterations=1)
        dilated_img = cv2.dilate(eroded_img, kernel, iterations=1)

        return dilated_img
    
    def hand_area_perimeter(self, img):
        area = 0
        perimeter = 0
        prev_pixel = 0
        
        height, width = img.shape[0:2]
        
        for i in range(height):
            for j in range(width):
                if img[i, j] == 255:
                    area += 1
                if img[i, j] != prev_pixel:
                    perimeter += 1
                prev_pixel = img[i, j]
                
        return area, perimeter
    
    def calculate_base_reference(self, image: np.ndarray) -> tuple:
        rows, cols = np.where(image > 0)
        bottom_row = np.max(rows)
        base_pixels = cols[rows == bottom_row]
        base_reference = (bottom_row, int(np.mean(base_pixels)))
        return base_reference
         
    def find_first_edge_pixel(self, image: np.ndarray) -> tuple:
        rows, cols = image.shape
        last_row = rows - 1
        for col in range(cols):
            if image[last_row, col] == 255:
                return last_row, col
        return None
    
    def is_edge_pixel(self, image: np.ndarray, row: int, col: int) -> bool:
        rows, cols = image.shape
        if row < 0 or row >= rows or col < 0 or col >= cols or image[row, col] == 0:
            return False
        neighbors = [
            image[row-1, col-1] if row-1 >= 0 and col-1 >= 0 else 0,
            image[row-1, col] if row-1 >= 0 else 0,
            image[row-1, col+1] if row-1 >= 0 and col+1 < cols else 0,
            image[row, col-1] if col-1 >= 0 else 0,
            image[row, col+1] if col+1 < cols else 0,
            image[row+1, col-1] if row+1 < rows and col-1 >= 0 else 0,
            image[row+1, col] if row+1 < rows else 0,
            image[row+1, col+1] if row+1 < rows and col+1 < cols else 0
        ]
        return any(n == 0 for n in neighbors)

    def find_contours(self, image: np.ndarray) -> np.ndarray:
        contour = []
        directions = [(-1, 0), (-1, 1), (0, 1), (1, 1), (1, 0), (1, -1), (0, -1), (-1, -1)]
        start_pixel = self.find_first_edge_pixel(image)
        current_pixel = start_pixel
        
        # print(start_pixel)
        current_direction = 0

        while True:
            contour.append(current_pixel)
            found_next_pixel = False
            
            for i in range(len(directions)):
                direction = directions[(current_direction + i) % len(directions)]
                next_pixel = (current_pixel[0] + direction[0], current_pixel[1] + direction[1])
                
                if self.is_edge_pixel(image, next_pixel[0], next_pixel[1]):
                    current_pixel = next_pixel
                    current_direction = (current_direction + i + 6) % len(directions)
                    found_next_pixel = True
                    break
            
            if not found_next_pixel or current_pixel == start_pixel:
                break

        return np.array(contour)

    def calculate_radial_distances(self, contour: np.array, reference_point: tuple) -> tuple:
        contour = contour.reshape(-1, 2)
        distances = np.sqrt((reference_point[0] - contour[:,0])**2 + (reference_point[1] - contour[:, 1])**2)
        return distances, contour
    
    def detect_peaks(self, distances: list, threshold_ratio=0.715) -> list:
        peaks = []
        indexes = [0]
        prev_distance = distances[0]
        prev_prev_distance = distances[1]
        threshold = threshold_ratio * max(distances)

        for i, distance in enumerate(distances):
            if prev_prev_distance <= prev_distance and prev_distance >= distance and prev_distance >= threshold:
                if (i - 1 - indexes[-1]) > 10:
                    peaks.append(prev_distance)
                    indexes.append(i-1)

            prev_prev_distance = prev_distance
            prev_distance = distance
        
        return peaks

    def check_classification(self, area: int, perimeter: int, peaks: np.array) -> None:
        norm_area = area/14400
        norm_perimeter = perimeter/760
        num_peaks = len(peaks)
        if (norm_area > 0.5 and norm_area < 0.8 and norm_perimeter > 0.6 and norm_perimeter < 0.83 and num_peaks == 1):
            print("RPi Classification: One finger up")
        elif (norm_area > 0.5 and norm_area < 0.8 and norm_perimeter > 0.6 and norm_perimeter < 0.83 and num_peaks == 2):
            print("RPi Classification: Victory")
        elif (norm_area > 0.5 and norm_area < 0.85 and norm_perimeter > 0.6 and norm_perimeter < 0.85 and num_peaks == 3):
            print("RPi Classification: Three fingers up")
        elif (norm_area > 0.8 and norm_perimeter > 0.8 and num_peaks == 4):
            print("RPi Classification: Four fingers up")
        elif (norm_area > 0.9 and norm_perimeter > 0.9 and num_peaks == 5):
            print("RPi Classification: Open palm")
        elif (norm_area < 0.7 and norm_perimeter < 0.6):
            print("RPi Classification: Closed fist")
        else:
            print("RPi Classification: Not recognized")