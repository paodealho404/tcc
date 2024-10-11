import cv2 as cv2
import numpy as np
img = cv2.imread('hand.jpg')
h = 240
w = 320
img = cv2.resize(img, (w, h))

img_r, img_g, img_b = cv2.split(img)
# cv2.imshow("Red", img_r)
# cv2.imshow("Green", img_g)
# cv2.imshow("Blue", img_b)
hx1 = np.vectorize(hex)

img_r = hx1(img_r.flatten())
img_g = hx1(img_g.flatten())
img_b = hx1(img_b.flatten())

img_r_txt = "const uint8_t img_r_channel[IMAGE_WIDTH*IMAGE_HEIGHT] = { "+ ','.join(img_r)+'};'
img_g_txt = "const uint8_t img_g_channel[IMAGE_WIDTH*IMAGE_HEIGHT] = { "+ ','.join(img_g)+'};'
img_b_txt = "const uint8_t img_b_channel[IMAGE_WIDTH*IMAGE_HEIGHT] = { "+ ','.join(img_b)+'};'

with open('image.c', 'w') as f:
    f.write('#include "image.h"\n\n')
    f.write(img_r_txt)
    f.write('\n\n')
    f.write(img_g_txt)
    f.write('\n\n')
    f.write(img_b_txt)
    f.write('\n\n')
    f.close()