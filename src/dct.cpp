#include "dct.h"

#include <array>
#include <cstdint>
#include <cmath>

int16_t mul_rshr(int16_t a, int16_t b, int sh) {
  return (int32_t(a) * b + (1 << (sh - 1))) >> sh;
}

void mul_cmpl_kara(int16_t a_r, int16_t a_i, int16_t b_r, int16_t b_i, int16_t& o_r, int16_t& o_i) {
  int16_t t1 = mul_rshr(a_r, b_r, 15);
  int16_t t2 = mul_rshr(a_i, b_i, 15);
  int16_t t3 = mul_rshr((a_r + a_i), int32_t(b_r + b_i) / 2, 15);
  o_r = t1 - t2;
  o_i = 2 * int(t3) - t1 - t2;
}

void dct(int16_t data[8]) {
  int16_t a1_r = mul_rshr(c32s[1], data[1], 15);
  int16_t a1_i = mul_rshr(c32s[7], data[1], 15);
  int16_t a7_r = mul_rshr(c32s[7], data[7], 15);
  int16_t a7_i = mul_rshr(c32s[1], data[7], 15);
  int16_t a3_r = mul_rshr(c32s[3], data[3], 15);
  int16_t a3_i = mul_rshr(c32s[5], data[3], 15);
  int16_t a5_r = mul_rshr(c32s[5], data[5], 15);
  int16_t a5_i = mul_rshr(c32s[3], data[5], 15);

  int16_t b1_r = a1_r + a5_r + a7_r + a3_r;
  int16_t b1_i = a1_i + a5_i - a7_i - a3_i;
  int16_t b5_r = a1_r - a5_i - a7_r - a3_i;
  int16_t b5_i = a1_i + a5_r + a7_i - a3_r;
  mul_cmpl_kara(b5_r, b5_i, c32s[2], c32s[6], b5_r, b5_i);

  int16_t a0 = mul_rshr(data[0], c32s[4], 15);
  int16_t a4 = mul_rshr(data[4], c32s[4], 15);
  int16_t b0 = a0 + a4;
  int16_t b4 = a0 - a4;
  int16_t b2, b6;
  mul_cmpl_kara(data[2], data[6], c32s[6], c32s[2], b6, b2);

  data[0] = b0 + b1_r + b2;
  data[1] = b4 + b5_r + b6;
  data[4] = b0 - b1_i - b2;
  data[5] = b4 - b5_i - b6;
  data[7] = b0 - b1_r + b2;
  data[6] = b4 - b5_r + b6;
  data[3] = b0 + b1_i - b2;
  data[2] = b4 + b5_i - b6;
}
