#include <cstdint>

#include "dct.h"

void dct_schoolbook(int16_t data[8]);

int16_t data[8];
int main() {
  for (int t = 0; t < 1000000000; t++) {
    dct(data);
    // dct_schoolbook(data);
  }
}
