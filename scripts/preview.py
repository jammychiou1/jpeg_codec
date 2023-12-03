import sys
import numpy as np
from matplotlib import pyplot as plt
with open(sys.argv[1], 'r') as f:
    lines = [line.strip() for line in f]
    data = [[int(a) for a in line.split()] for line in lines]
Y, X = data[0]
img = np.zeros((Y, X, 3))
for k in range(3):
    for i, row in enumerate(data[1 + Y * k : 1 + Y * (k + 1)]):
        # row = np.ndarray(row)
        # print(row.shape)
        print(row)
        img[i, :, k] = row
print(img)
plt.imshow(img[:, :, 0], cmap='gray')
plt.show()
plt.imshow(img[:, :, 1], cmap='gray')
plt.show()
plt.imshow(img[:, :, 2], cmap='gray')
plt.show()

img2 = np.zeros((Y, X, 3))
for i in range(Y):
    for j in range(X):
        img2[i, j, 0] = img[i, j, 0] + 1.402 * (img[i, j, 2] - 128)
        img2[i, j, 1] = img[i, j, 0] - 0.344136 * (img[i, j, 1] - 128) - 0.714136 * (img[i, j, 2] - 128)
        img2[i, j, 2] = img[i, j, 0] + 1.772 * (img[i, j, 1] - 128)
print(img2)
plt.imshow(img2 / 255)
plt.show()

