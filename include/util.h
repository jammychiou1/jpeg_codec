#ifndef UTIL_H
#define UTIL_H

#include <iomanip>
#include <iostream>

uint64_t low_k_mask(int k) {
  uint64_t pow = ((uint64_t)1) << k;
  return pow - 1;
}

void print_byte(std::ostream& os, int byte) {
  os << std::hex << std::setfill('0') << std::setw(2) << byte << std::dec;
}

void print_binary(std::ostream& os, uint64_t data, int len) {
  for (int i = len - 1; i >= 0; i--) {
    os << ((data >> i) & 1);
  }
}

#endif // UTIL_H
