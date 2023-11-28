#ifndef DCT_H
#define DCT_H

#include <cstdint>

void idct(int16_t in[8][8], uint8_t out[8][8]);

#include "dct_impl.h"

#if DCT_IMPL == DCT_IMPL_SCHOOLBOOK
#include "dct_schoolbook.h"
#elif DCT_IMPL == DCT_IMPL_FFT
#include "dct_fft.h"
#else
#include "dct_ssse3.h"
#endif

#include <algorithm>

uint8_t lvl_shift_clip(int a) {
  a += 128;
  a = std::min(a, 255);
  a = std::max(a, 0);
  return a;
}

#if DCT_IMPL == DCT_IMPL_SCHOOLBOOK || DCT_IMPL == DCT_IMPL_FFT

void idct(int16_t in[8][8], uint8_t out[8][8]) {
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

#endif

#endif // DCT_H

