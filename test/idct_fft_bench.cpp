#include <cstdint>

#include "dct_impl.h"
#define DCT_IMPL DCT_IMPL_FFT
#include "dct.h"

int16_t in[8][8];
uint8_t out[8][8];
int main() {
  for (int t = 0; t < 100000000; t++) {
    idct(in, out);
  }
}
