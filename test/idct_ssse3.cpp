#include <cstdint>
#include <iostream>
#include <iomanip>
#include <tmmintrin.h>

#include "dct_impl.h"
#define DCT_IMPL DCT_IMPL_SSSE3
#include "dct.h"

int16_t in[8][8];
uint8_t out[8][8];
int main() {
  using std::cout;
  in[1][1] = 300;
  idct(in, out);
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      cout << std::setw(3) << int(out[i][j]) << ' ';
    }
    cout << '\n';
  }
}
