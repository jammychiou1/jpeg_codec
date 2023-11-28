#ifndef DCT_H
#define DCT_H

#include <array>
#include <cmath>
#include <cstdint>

const double pi = std::acos(-1);
const std::array<int16_t, 8> c32s = [] {
  std::array<int16_t, 8> res{};
  for (int i = 1; i < 8; i++) {
    res[i] = std::round(std::cos(2 * pi * i / 32) * (1 << 15));
  }
  return res;
} ();

void dct(int16_t data[8]);

#endif // DCT_H

