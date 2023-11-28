#include <cstdint>
#include <iostream>
#include <iomanip>

#include "dct_simd.h"

int16_t data[8][8];
int main() {
  using std::cout;
  for (int t = 0; t < 8; t++) {
    for (int i = 0; i < 8; i++) {
      data[i][t] = i == t ? 1024 * (1 << 3) : 0;
    }
  }

  int16x8_t s0 = _mm_load_si128((int16x8_t*) &data[0]);
  int16x8_t s1 = _mm_load_si128((int16x8_t*) &data[1]);
  int16x8_t s2 = _mm_load_si128((int16x8_t*) &data[2]);
  int16x8_t s3 = _mm_load_si128((int16x8_t*) &data[3]);
  int16x8_t s4 = _mm_load_si128((int16x8_t*) &data[4]);
  int16x8_t s5 = _mm_load_si128((int16x8_t*) &data[5]);
  int16x8_t s6 = _mm_load_si128((int16x8_t*) &data[6]);
  int16x8_t s7 = _mm_load_si128((int16x8_t*) &data[7]);

  dct_simd(s0, s1, s2, s3, s4, s5, s6, s7,
      s0, s1, s2, s3, s4, s5, s6, s7);

  _mm_store_si128((int16x8_t*) &data[0], s0);
  _mm_store_si128((int16x8_t*) &data[1], s1);
  _mm_store_si128((int16x8_t*) &data[2], s2);
  _mm_store_si128((int16x8_t*) &data[3], s3);
  _mm_store_si128((int16x8_t*) &data[4], s4);
  _mm_store_si128((int16x8_t*) &data[5], s5);
  _mm_store_si128((int16x8_t*) &data[6], s6);
  _mm_store_si128((int16x8_t*) &data[7], s7);

  for (int t = 0; t < 8; t++) {
    for (int i = 0; i < 8; i++) {
      cout << std::setw(6) << std::fixed << std::setprecision(0) << double(data[i][t]) / (1 << 3) / 2 << ' ';
    }
    cout << '\n';
  }
}
