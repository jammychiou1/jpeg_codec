#include <cstdint>
#include <tmmintrin.h>

#include "dct_ssse3.h"

int16_t data[8][8];
int main() {
  for (int t = 0; t < 100000000; t++) {
    int16x8_t s0 = _mm_load_si128((int16x8_t*) &data[0]);
    int16x8_t s1 = _mm_load_si128((int16x8_t*) &data[1]);
    int16x8_t s2 = _mm_load_si128((int16x8_t*) &data[2]);
    int16x8_t s3 = _mm_load_si128((int16x8_t*) &data[3]);
    int16x8_t s4 = _mm_load_si128((int16x8_t*) &data[4]);
    int16x8_t s5 = _mm_load_si128((int16x8_t*) &data[5]);
    int16x8_t s6 = _mm_load_si128((int16x8_t*) &data[6]);
    int16x8_t s7 = _mm_load_si128((int16x8_t*) &data[7]);

    dct_ssse3(s0, s1, s2, s3, s4, s5, s6, s7,
        s0, s1, s2, s3, s4, s5, s6, s7);

    _mm_store_si128((int16x8_t*) &data[0], s0);
    _mm_store_si128((int16x8_t*) &data[1], s1);
    _mm_store_si128((int16x8_t*) &data[2], s2);
    _mm_store_si128((int16x8_t*) &data[3], s3);
    _mm_store_si128((int16x8_t*) &data[4], s4);
    _mm_store_si128((int16x8_t*) &data[5], s5);
    _mm_store_si128((int16x8_t*) &data[6], s6);
    _mm_store_si128((int16x8_t*) &data[7], s7);
  }
}
