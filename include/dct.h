#ifndef DCT_H
#define DCT_H

#include <cstdint>

static void idct(int16_t in[8][8], uint8_t out[8][8]);

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

static uint8_t lvl_shift_clip(int a) {
  a += 1 << 4;
  a >>= 5;
  a += 128;
  a = std::min(a, 255);
  a = std::max(a, 0);
  return a;
}

#if DCT_IMPL == DCT_IMPL_SCHOOLBOOK || DCT_IMPL == DCT_IMPL_FFT

static void idct(int16_t in[8][8], uint8_t out[8][8]) {
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

// #include <iostream>

static void idct(int16_t in[8][8], uint8_t out[8][8]) {
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

  idct_inter_vector(s0, s1, s2, s3, s4, s5, s6, s7,
      s0, s1, s2, s3, s4, s5, s6, s7);

  // _mm_store_si128((int16x8_t*) &in[0], s0);
  // _mm_store_si128((int16x8_t*) &in[1], s1);
  // _mm_store_si128((int16x8_t*) &in[2], s2);
  // _mm_store_si128((int16x8_t*) &in[3], s3);
  // _mm_store_si128((int16x8_t*) &in[4], s4);
  // _mm_store_si128((int16x8_t*) &in[5], s5);
  // _mm_store_si128((int16x8_t*) &in[6], s6);
  // _mm_store_si128((int16x8_t*) &in[7], s7);

  // for (int i = 0; i < 8; i++) {
  //   for (int j = 0; j < 8; j++) {
  //     std::cerr << in[i][j] << ' ';
  //   }
  //   std::cerr << '\n';
  // }

  idct_intra_vector(s0, s0);
  idct_intra_vector(s1, s1);
  idct_intra_vector(s2, s2);
  idct_intra_vector(s3, s3);
  idct_intra_vector(s4, s4);
  idct_intra_vector(s5, s5);
  idct_intra_vector(s6, s6);
  idct_intra_vector(s7, s7);

  _mm_store_si128((int16x8_t*) &in[0], s0);
  _mm_store_si128((int16x8_t*) &in[1], s1);
  _mm_store_si128((int16x8_t*) &in[2], s2);
  _mm_store_si128((int16x8_t*) &in[3], s3);
  _mm_store_si128((int16x8_t*) &in[4], s4);
  _mm_store_si128((int16x8_t*) &in[5], s5);
  _mm_store_si128((int16x8_t*) &in[6], s6);
  _mm_store_si128((int16x8_t*) &in[7], s7);

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      out[i][j] = lvl_shift_clip(in[i][j]);
    }
  }
}

#endif

#endif // DCT_H

