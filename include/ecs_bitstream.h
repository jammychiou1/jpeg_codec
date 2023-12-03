#ifndef ECS_BITSTREAM_H
#define ECS_BITSTREAM_H

#include <cstdint>
#include <stdexcept>

#include "huffman.h"
#include "util.h"

struct ecs_bitstream {
  const uint8_t* ptr;
  int next;
  int begin;
  int end;
  uint64_t bitbuf;
  int bitcnt;

  ecs_bitstream(const uint8_t* ptr = nullptr, int begin = 0, int end = 0)
      : ptr(ptr), begin(begin), end(end), next(begin), bitbuf(0), bitcnt(0) {}

  bool has_more() { return next < end; }

  void more() {
    if (!has_more()) return;
    if (bitcnt + 8 >= 64) throw std::runtime_error("ecs_bitstream bitbuf full");
    bitbuf = (bitbuf << 8) | ptr[next];
    bitcnt += 8;
    if (ptr[next] == 0xff) next += 2;
    else next++;
  }

  uint64_t peak_k(int k) {
    if (k == 0) return 0;
    while (bitcnt < k) {
      if (!has_more()) return (bitbuf << (k - bitcnt)) & low_k_mask(k);
      more();
    }
    return (bitbuf >> (bitcnt - k)) & low_k_mask(k);
  }

  uint64_t get_k(int k) {
    if (k == 0) return 0;
    while (bitcnt < k) {
      if (!has_more()) throw std::runtime_error("no more bits");
      more();
    }
    uint64_t result = (bitbuf >> (bitcnt - k)) & low_k_mask(k);
    bitcnt -= k;
    return result;
  }

  int get_huffman(const huffman_lut* htab);
};

#endif // ENTROPY_CODED_H
