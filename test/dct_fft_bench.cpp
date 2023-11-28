#include <cstdint>

#include "dct_fft.h"

int16_t data[8][8];
int main() {
  for (int t = 0; t < 100000000; t++) {
    for (int i = 0; i < 8; i++) {
      idct_1d(data[i], data[i]);
    }
  }
}
