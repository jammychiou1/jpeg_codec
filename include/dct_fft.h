#ifndef DCT_FFT_H
#define DCT_FFT_H

#include <array>
#include <cstdint>
#include <cmath>

const double pi = std::acos(-1);
const std::array<int16_t, 8> c32s = [] {
  std::array<int16_t, 8> res{};
  for (int i = 1; i < 8; i++) {
    res[i] = std::round(std::cos(2 * pi * i / 32) * (1 << 15));
  }
  return res;
} ();

static int16_t mul_rshr(int16_t a, int16_t b, int sh) {
  return (int32_t(a) * b + (1 << (sh - 1))) >> sh;
}

static void mul_cmpl_kara(int16_t a_r, int16_t a_i, int16_t b_r, int16_t b_i, int16_t& o_r, int16_t& o_i) {
  int16_t t1 = mul_rshr(a_r, b_r, 15);
  int16_t t2 = mul_rshr(a_i, b_i, 15);
  int16_t t3 = mul_rshr((a_r + a_i), int32_t(b_r + b_i) / 2, 15);
  o_r = t1 - t2;
  o_i = 2 * int(t3) - t1 - t2;
}

static void idct_1d(int16_t in[8], int16_t out[8]) {
  int16_t a1_r = mul_rshr(c32s[1], in[1], 15);
  int16_t a1_i = mul_rshr(c32s[7], in[1], 15);
  int16_t a7_r = mul_rshr(c32s[7], in[7], 15);
  int16_t a7_i = mul_rshr(c32s[1], in[7], 15);
  int16_t a3_r = mul_rshr(c32s[3], in[3], 15);
  int16_t a3_i = mul_rshr(c32s[5], in[3], 15);
  int16_t a5_r = mul_rshr(c32s[5], in[5], 15);
  int16_t a5_i = mul_rshr(c32s[3], in[5], 15);

  int16_t b1_r = a1_r + a5_r + a7_r + a3_r;
  int16_t b1_i = a1_i + a5_i - a7_i - a3_i;
  int16_t b5_r = a1_r - a5_i - a7_r - a3_i;
  int16_t b5_i = a1_i + a5_r + a7_i - a3_r;
  mul_cmpl_kara(b5_r, b5_i, c32s[2], c32s[6], b5_r, b5_i);

  int16_t a0 = mul_rshr(in[0], c32s[4], 15);
  int16_t a4 = mul_rshr(in[4], c32s[4], 15);
  int16_t b0 = a0 + a4;
  int16_t b4 = a0 - a4;
  int16_t b2, b6;
  mul_cmpl_kara(in[2], in[6], c32s[6], c32s[2], b6, b2);

  out[0] = b0 + b1_r + b2;
  out[1] = b4 + b5_r + b6;
  out[4] = b0 - b1_i - b2;
  out[5] = b4 - b5_i - b6;
  out[7] = b0 - b1_r + b2;
  out[6] = b4 - b5_r + b6;
  out[3] = b0 + b1_i - b2;
  out[2] = b4 + b5_i - b6;
}

#endif // DCT_FFT_H
