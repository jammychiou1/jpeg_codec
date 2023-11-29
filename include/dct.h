#ifndef DCT_H
#define DCT_H

#include <cstdint>

void idct(int16_t in[8][8], uint8_t out[8][8]);

#include "dct_impl.h"

#ifndef DCT_IMPL
#define DCT_IMPL DCT_IMPL_SCHOOLBOOK
#endif

#if DCT_IMPL == DCT_IMPL_SCHOOLBOOK
#include "dct_schoolbook.h"
#elif DCT_IMPL == DCT_IMPL_FFT
#include "dct_fft.h"
#else
#include "dct_ssse3.h"
#endif

#include <algorithm>

uint8_t lvl_shift_clip(int a) {
  a += 1 << 4;
  a >>= 5;
  a += 128;
  a = std::min(a, 255);
  a = std::max(a, 0);
  return a;
}

#if DCT_IMPL == DCT_IMPL_SCHOOLBOOK || DCT_IMPL == DCT_IMPL_FFT

void idct(int16_t in[8][8], uint8_t out[8][8]) {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      in[i][j] <<= 3;
    }
  }
  for (int i = 0; i < 8; i++) {
    idct_1d(in[i], in[i]);
  }
  for (int j = 0; j < 8; j++) {
    int16_t tmp[8];
    for (int i = 0; i < 8; i++) {
      tmp[i] = in[i][j];
    }
    idct_1d(tmp, tmp);
    for (int i = 0; i < 8; i++) {
      out[i][j] = lvl_shift_clip(tmp[i]);
    }
  }
}

#else

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

void idct_1d(int16_t in[8], int16_t out[8]) {
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

void idct(int16_t in[8][8], uint8_t out[8][8]) {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      in[i][j] <<= 3;
    }
  }

  int16x8_t s0 = _mm_load_si128((int16x8_t*) &in[0]);
  int16x8_t s1 = _mm_load_si128((int16x8_t*) &in[1]);
  int16x8_t s2 = _mm_load_si128((int16x8_t*) &in[2]);
  int16x8_t s3 = _mm_load_si128((int16x8_t*) &in[3]);
  int16x8_t s4 = _mm_load_si128((int16x8_t*) &in[4]);
  int16x8_t s5 = _mm_load_si128((int16x8_t*) &in[5]);
  int16x8_t s6 = _mm_load_si128((int16x8_t*) &in[6]);
  int16x8_t s7 = _mm_load_si128((int16x8_t*) &in[7]);

  idct_ssse3(s0, s1, s2, s3, s4, s5, s6, s7,
      s0, s1, s2, s3, s4, s5, s6, s7);

  _mm_store_si128((int16x8_t*) &in[0], s0);
  _mm_store_si128((int16x8_t*) &in[1], s1);
  _mm_store_si128((int16x8_t*) &in[2], s2);
  _mm_store_si128((int16x8_t*) &in[3], s3);
  _mm_store_si128((int16x8_t*) &in[4], s4);
  _mm_store_si128((int16x8_t*) &in[5], s5);
  _mm_store_si128((int16x8_t*) &in[6], s6);
  _mm_store_si128((int16x8_t*) &in[7], s7);

  for (int i = 0; i < 8; i++) {
    idct_1d(in[i], in[i]);
    for (int j = 0; j < 8; j++) {
      out[i][j] = lvl_shift_clip(in[i][j]);
    }
  }
}

#endif

#endif // DCT_H

