#ifndef DCT_SSSE3_H
#define DCT_SSSE3_H

#include <array>
#include <cmath>
#include <cstdint>
#include <tmmintrin.h>
#include <emmintrin.h>

const double pi = std::acos(-1);
const std::array<int16_t, 32> c32s = [] {
  std::array<int16_t, 32> res{};
  for (int i = 1; i < 32; i++) {
    res[i] = std::round(std::cos(2 * pi * i / 32) * (1 << 15));
  }
  return res;
} ();

const std::array<std::array<int16_t, 8>, 8> bases = [] {
  std::array<std::array<int16_t, 8>, 8> res{};
  for (int j = 0; j < 8; j++) {
    res[0][j] = c32s[4];
  }
  for (int i = 1; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      res[i][j] = c32s[(2 * j + 1) * i % 32];
    }
  }
  return res;
} ();

typedef __m128i int16x8_t;

static int16x8_t rdmulh(int16x8_t v, int16_t scl) {
  return _mm_mulhrs_epi16(v, _mm_set1_epi16(scl));
}

static void mul_cmpl_kara(int16x8_t a_r, int16x8_t a_i, int16_t b_r, int16_t b_i, int16x8_t& o_r, int16x8_t& o_i) {
  int16x8_t t1 = rdmulh(a_r, b_r);
  int16x8_t t2 = rdmulh(a_i, b_i);
  int16x8_t t3 = rdmulh(_mm_add_epi16(a_r, a_i), int(b_r + b_i) / 2);
  o_r = _mm_sub_epi16(t1, t2);
  o_i = _mm_sub_epi16(_mm_slli_epi16(t3, 1), _mm_add_epi16(t1, t2));
}

static void idct_inter_vector(
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

// #include <iostream>
// void dump(int16x8_t v) {
//   int16_t tmp[8];
//   _mm_store_si128((int16x8_t*) &tmp[0], v);
//   for (int j = 0; j < 8; j++) {
//     std::cerr << tmp[j] << ' ';
//   }
//   std::cerr << '\n';
// }

static void idct_intra_vector(int16x8_t s, int16x8_t& x) {
  int16x8_t b0 = _mm_load_si128((int16x8_t*) &bases[0]);
  // dump(b0);
  x = rdmulh(b0, _mm_extract_epi16(s, 0));
  // dump(x);

  int16x8_t b1 = _mm_load_si128((int16x8_t*) &bases[1]);
  // dump(b1);
  x = _mm_add_epi16(x, rdmulh(b1, _mm_extract_epi16(s, 1)));
  // dump(x);

  int16x8_t b2 = _mm_load_si128((int16x8_t*) &bases[2]);
  x = _mm_add_epi16(x, rdmulh(b2, _mm_extract_epi16(s, 2)));

  int16x8_t b3 = _mm_load_si128((int16x8_t*) &bases[3]);
  x = _mm_add_epi16(x, rdmulh(b3, _mm_extract_epi16(s, 3)));

  int16x8_t b4 = _mm_load_si128((int16x8_t*) &bases[4]);
  x = _mm_add_epi16(x, rdmulh(b4, _mm_extract_epi16(s, 4)));

  int16x8_t b5 = _mm_load_si128((int16x8_t*) &bases[5]);
  x = _mm_add_epi16(x, rdmulh(b5, _mm_extract_epi16(s, 5)));

  int16x8_t b6 = _mm_load_si128((int16x8_t*) &bases[6]);
  x = _mm_add_epi16(x, rdmulh(b6, _mm_extract_epi16(s, 6)));

  int16x8_t b7 = _mm_load_si128((int16x8_t*) &bases[7]);
  x = _mm_add_epi16(x, rdmulh(b7, _mm_extract_epi16(s, 7)));

  // for (int i = 1; i < 8; i++) {
  //   int16x8_t bi = _mm_load_si128((int16x8_t*) &bases[i]);
  //   x = _mm_add_epi16(x, _mm_mulhrs_epi16(s, bi));
  // }
}

#endif // DCT_SSSE3_H

