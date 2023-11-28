#include <cstdint>
#include <iostream>
#include <iomanip>

#include "dct_fft.h"

int16_t data[8];
int main() {
  using std::cout;
  for (int t = 0; t < 8; t++) {
    for (int i = 0; i < 8; i++) {
      data[i] = i == t ? 1024 * (1 << 4) : 0;
    }
    idct_1d(data, data);
    for (int i = 0; i < 8; i++) {
      cout << std::setw(6) << std::fixed << std::setprecision(0) << double(data[i]) / (1 << 4) / 2 << ' ';
    }
    cout << '\n';
  }
}
