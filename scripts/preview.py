import sys
import numpy as np
from matplotlib import pyplot as plt
with open(sys.argv[1], 'r') as f:
    lines = [line.strip() for line in f]
    data = [[int(a) for a in line.split()] for line in lines]

Y = 480
X = 640
img = np.zeros((Y, X))
now = 0
for i, y in enumerate(range(0, Y, 8)):
    for j, x in enumerate(range(0, X, 8)):
        img[y : y + 8, x : x + 8] = data[now : now + 8]
        now += 8
print(img)
plt.imshow(img, cmap='gray')
plt.show()

