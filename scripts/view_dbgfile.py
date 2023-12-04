import sys
import numpy as np
from matplotlib import pyplot as plt

with open(sys.argv[1], 'r') as f:
    lines = [line.strip() for line in f]
    data = [[int(a) for a in line.split()] for line in lines]

Y, X = data[0]
ycbcr = np.zeros((Y, X, 3))
for k in range(3):
    for i, row in enumerate(data[1 + Y * k : 1 + Y * (k + 1)]):
        ycbcr[i, :, k] = row

plt.imshow(ycbcr[:, :, 0] / 255)
plt.show()
plt.imshow(ycbcr[:, :, 1] / 255)
plt.show()
plt.imshow(ycbcr[:, :, 2] / 255)
plt.show()

rgb = np.zeros((Y, X, 3))
for i in range(Y):
    for j in range(X):
        rgb[i, j, 0] = ycbcr[i, j, 0] + 1.402 * (ycbcr[i, j, 2] - 128)
        rgb[i, j, 1] = ycbcr[i, j, 0] - 0.344136 * (ycbcr[i, j, 1] - 128) - 0.714136 * (ycbcr[i, j, 2] - 128)
        rgb[i, j, 2] = ycbcr[i, j, 0] + 1.772 * (ycbcr[i, j, 1] - 128)

plt.imshow(rgb / 255)
plt.show()
