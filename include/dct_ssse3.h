#ifndef DCT_SSSE3_H
#define DCT_SSSE3_H

#include <array>
#include <cmath>
#include <cstdint>
#include <tmmintrin.h>

const double pi = std::acos(-1);
const std::array<int16_t, 8> c32s = [] {
  std::array<int16_t, 8> res{};
  for (int i = 1; i < 8; i++) {
    res[i] = std::round(std::cos(2 * pi * i / 32) * (1 << 15));
  }
  return res;
} ();

typedef __m128i int16x8_t;

int16x8_t rdmulh(int16x8_t v, int16_t scl) {
  return _mm_mulhrs_epi16(v, _mm_set1_epi16(scl));
}

void mul_cmpl_kara(int16x8_t a_r, int16x8_t a_i, int16_t b_r, int16_t b_i, int16x8_t& o_r, int16x8_t& o_i) {
  int16x8_t t1 = rdmulh(a_r, b_r);
  int16x8_t t2 = rdmulh(a_i, b_i);
  int16x8_t t3 = rdmulh(_mm_add_epi16(a_r, a_i), int(b_r + b_i) / 2);
  o_r = _mm_sub_epi16(t1, t2);
  o_i = _mm_sub_epi16(_mm_slli_epi16(t3, 1), _mm_add_epi16(t1, t2));
}

void dct_ssse3(
    int16x8_t s0, int16x8_t s1, int16x8_t s2, int16x8_t s3,
    int16x8_t s4, int16x8_t s5, int16x8_t s6, int16x8_t s7,
    int16x8_t& x0, int16x8_t& x1, int16x8_t& x2, int16x8_t& x3,
    int16x8_t& x4, int16x8_t& x5, int16x8_t& x6, int16x8_t& x7) {

  int16x8_t a1_r = rdmulh(s1, c32s[1]);
  int16x8_t a1_i = rdmulh(s1, c32s[7]);
  int16x8_t a7_r = rdmulh(s7, c32s[7]);
  int16x8_t a7_i = rdmulh(s7, c32s[1]);
  int16x8_t a3_r = rdmulh(s3, c32s[3]);
  int16x8_t a3_i = rdmulh(s3, c32s[5]);
  int16x8_t a5_r = rdmulh(s5, c32s[5]);
  int16x8_t a5_i = rdmulh(s5, c32s[3]);

  int16x8_t b1_r = _mm_add_epi16(_mm_add_epi16(a1_r, a7_r), _mm_add_epi16(a5_r, a3_r));
  int16x8_t b1_i = _mm_add_epi16(_mm_sub_epi16(a1_i, a7_i), _mm_sub_epi16(a5_i, a3_i));
  int16x8_t b5_r = _mm_sub_epi16(_mm_sub_epi16(a1_r, a7_r), _mm_add_epi16(a5_i, a3_i));
  int16x8_t b5_i = _mm_add_epi16(_mm_add_epi16(a1_i, a7_i), _mm_sub_epi16(a5_r, a3_r));
  mul_cmpl_kara(b5_r, b5_i, c32s[2], c32s[6], b5_r, b5_i);

  int16x8_t a0 = rdmulh(s0, c32s[4]);
  int16x8_t a4 = rdmulh(s4, c32s[4]);
  int16x8_t b0 = _mm_add_epi16(a0, a4);
  int16x8_t b4 = _mm_sub_epi16(a0, a4);
  int16x8_t b2, b6;
  mul_cmpl_kara(s2, s6, c32s[6], c32s[2], b6, b2);

  int16x8_t c0 = _mm_add_epi16(b0, b2);
  int16x8_t c2 = _mm_sub_epi16(b0, b2);
  int16x8_t c4 = _mm_add_epi16(b4, b6);
  int16x8_t c6 = _mm_sub_epi16(b4, b6);

  x0 = _mm_add_epi16(c0, b1_r);
  x1 = _mm_add_epi16(c4, b5_r);
  x4 = _mm_sub_epi16(c2, b1_i);
  x5 = _mm_sub_epi16(c6, b5_i);
  x7 = _mm_sub_epi16(c0, b1_r);
  x6 = _mm_sub_epi16(c4, b5_r);
  x3 = _mm_add_epi16(c2, b1_i);
  x2 = _mm_add_epi16(c6, b5_i);
}

#endif // DCT_SSSE3_H

