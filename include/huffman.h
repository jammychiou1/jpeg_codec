#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <array>
#include <variant>
#include <vector>

struct huffman_lut;

struct huffman_codeword {
  int len;
  int val;
};
struct huffman_supernode {
  huffman_lut* next;
};

struct huffman_lut {
  std::array<std::variant<huffman_supernode, huffman_codeword>, 256> table;

  void add_codeword(int key, int len, int val) {
    std::cerr << "add codeword " << std::hex << key << std::dec << ' ' << len << ' ' << val << '\n';
    huffman_lut* nd = this;
    int len_now = len;
    while (len_now > 8) {
      int prefix = (key >> (len_now - 8)) & 0xff;
      if (std::holds_alternative<huffman_codeword>(nd->table[prefix])) {
        throw std::runtime_error("prefix used by codeword");
      }
      if (!std::get<huffman_supernode>(nd->table[prefix]).next) {
        huffman_supernode supernode{new huffman_lut()};
        nd->table[prefix] = supernode;
        std::cerr << (void*)nd << '[' << std::hex << prefix << std::dec << "] <- " << (void*)supernode.next << '\n';
      }
      nd = std::get<huffman_supernode>(nd->table[prefix]).next;
      len_now -= 8;
    }
    int last = key % (1 << len_now);
    for (int i = last << (8 - len_now); i < (last + 1) << (8 - len_now); i++) {
      if (std::holds_alternative<huffman_codeword>(nd->table[i])) {
        throw std::runtime_error("lut entry used by codeword");
      }
      if (std::get<huffman_supernode>(nd->table[i]).next) {
        throw std::runtime_error("lut entry used by supernode");
      }
      nd->table[i] = huffman_codeword{len, val};
      std::cerr << (void*)nd << '[' << std::hex << i << std::dec << "] <- (" << len << ", " << val << ")\n";
    }
  }
};

#endif // HUFFMAN_H
