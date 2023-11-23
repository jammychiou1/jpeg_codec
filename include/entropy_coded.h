#ifndef ENTROPY_CODED_H
#define ENTROPY_CODED_H

#include <cstdint>

#include "mmap.h"
#include "util.h"

struct unstuffing_bitstream {
  mapped_file mmap;
  int next_idx;
  int end;

  uint64_t bitbuf;
  int bitcnt;

  void set_mmap(mapped_file m, int l, int r) {
    mmap = m;
    next_idx = l;
    end = r;
    bitbuf = 0;
    bitcnt = 0;
  }
  bool has_more() {
    return next_idx < end;
  }
  void more() {
    // print_binary(std::cerr, bitbuf, bitcnt);
    // std::cerr << " -> ";

    if (!has_more()) {
      return;
    }
    bitbuf = (bitbuf << 8) | mmap[next_idx];
    bitcnt += 8;

    // print_binary(std::cerr, bitbuf, bitcnt);
    // std::cerr << '\n';

    if (mmap[next_idx] == 0xff) {
      next_idx += 2;
    }
    else {
      next_idx++;
    }
  }
  uint64_t peak_k(int k) {
    if (k == 0) {
      return 0;
    }
    while (bitcnt < k) {
      if (!has_more()) {
        return (bitbuf << (k - bitcnt)) & low_k_mask(k);
      }
      more();
    }
    return (bitbuf >> (bitcnt - k)) & low_k_mask(k);
  }
  uint64_t get_k(int k) {
    if (k == 0) {
      return 0;
    }
    while (bitcnt < k) {
      if (!has_more()) {
        throw std::runtime_error("no more bits");
      }
      more();
    }
    uint64_t result = (bitbuf >> (bitcnt - k)) & low_k_mask(k);
    bitcnt -= k;
    return result;
  }
};

#endif // ENTROPY_CODED_H
