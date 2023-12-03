#include "huffman.h"

#include <stdexcept>

void huffman_lut::add_codeword(int key, int len, int val) {
  // std::cerr << "add codeword ";
  // print_binary(std::cerr, key, len);
  // std::cerr << " -> " << val << '\n';

  huffman_lut* nd = this;
  int len_now = len;
  while (len_now > 8) {
    int prefix = (key >> (len_now - 8)) & 0xff;
    huffman_entry& entry = nd->entries[prefix];
    if (entry.is_cw) throw std::runtime_error("prefix used by codeword");
    huffman_lut* next = entry.u.next;
    if (!next) {
      next = new huffman_lut();
      *next = {};
      entry.u.next = next;

      // std::cerr << (void*)nd << '[';
      // print_byte(std::cerr, prefix);
      // std:: cerr << "] <- " << (void*)supernode.next << '\n';
    }
    nd = next;
    len_now -= 8;
  }

  int last = key % (1 << len_now);
  int begin = last << (8 - len_now);
  int end = (last + 1) << (8 - len_now);
  for (int i = begin; i < end; i++) {
    if (nd->entries[i].is_cw) throw std::runtime_error("lut entry used by codeword");
    if (nd->entries[i].u.next) throw std::runtime_error("lut entry used by supernode");
    nd->entries[i].is_cw = true;
    nd->entries[i].u.cw = {len, val};

    // std::cerr << (void*)nd << '[';
    // print_byte(std::cerr, i);
    // std::cerr << "] <- ";
    // print_byte(std::cerr, val);
    // std::cerr << '\n';
  }
}
