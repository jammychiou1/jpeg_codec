#include <emmintrin.h>
#include <cstdint>
#include <iostream>

int16_t in1[8];
int16_t in2[8];
int16_t out[8];

typedef __m128i int16x8_t;

using std::cin, std::cout;
int main() {
  for (int i = 0; i < 8; i++) {
    cin >> in1[i];
  }
  for (int i = 0; i < 8; i++) {
    cin >> in2[i];
  }
  int16x8_t vec1 = _mm_load_si128((int16x8_t*) &in1);
  int16x8_t vec2 = _mm_load_si128((int16x8_t*) &in2);
  int16x8_t vec3 = _mm_mullo_epi16(vec1, vec2);
  _mm_store_si128((int16x8_t*) &out, vec3);
  for (int i = 0; i < 8; i++) {
    cout << out[i] << " \n"[i == 7];
  }
}
