#include "ecs_bitstream.h"

#include <stdexcept>

#include "huffman.h"

int ecs_bitstream::get_huffman(const huffman_lut* htab) {
  int dep = 0;
  while (true) {
    if (!htab) throw std::runtime_error("no codeword found");
    int prefix = peak_k(8 * (dep + 1));
    int key = prefix & 0xff;
    huffman_entry entry = htab->entries[key];
    if (entry.is_cw) {
      huffman_codeword cw = entry.u.cw;
      get_k(cw.len);
      return cw.val;
    }
    htab = entry.u.next;
    dep++;
  }
}
