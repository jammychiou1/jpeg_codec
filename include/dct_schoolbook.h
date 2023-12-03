#ifndef DCT_SCHOOLBOOK_H
#define DCT_SCHOOLBOOK_H

#include <array>
#include <cstdint>
#include <cmath>

const double pi = std::acos(-1);
const std::array<int16_t, 32> c32s = [] {
  std::array<int16_t, 32> res{};
  for (int i = 1; i < 32; i++) {
    res[i] = std::round(std::cos(2 * pi * i / 32) * (1 << 15));
  }
  return res;
} ();

static int16_t mul_rshr(int16_t a, int16_t b, int sh) {
  return (int32_t(a) * b + (1 << (sh - 1))) >> sh;
}

static void idct_1d(int16_t in[8], int16_t out[8]) {
  int16_t tmp[8] = {}; // to support inplace
  for (int i = 0; i < 8; i++) {
    tmp[i] += mul_rshr(in[0], c32s[4], 15);
  }
  for (int t = 1; t < 8; t++) {
    for (int i = 0; i < 8; i++) {
      tmp[i] += mul_rshr(in[t], c32s[(2 * i + 1) * t % 32], 15);
    }
  }
  for (int i = 0; i < 8; i++) {
    out[i] = tmp[i];
  }
}

#endif // DCT_SCHOOLBOOK_H
