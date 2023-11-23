#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <array>
#include <variant>
#include <vector>


#include <iostream>
#include "util.h"

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
    // std::cerr << "add codeword ";
    // print_binary(std::cerr, key, len);
    // std::cerr << " -> " << val << '\n';

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

        // std::cerr << (void*)nd << '[';
        // print_byte(std::cerr, prefix);
        // std:: cerr << "] <- " << (void*)supernode.next << '\n';
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

      // std::cerr << (void*)nd << '[';
      // print_byte(std::cerr, i);
      // std::cerr << "] <- ";
      // print_byte(std::cerr, val);
      // std::cerr << '\n';
    }
  }

  template<class T>
  int lookup(T& bs) const {
    huffman_codeword codeword;
    int key1 = bs.peak_k(8);

    // std::cerr << "key1: ";
    // print_byte(std::cerr, key1);
    // std::cerr << '\n';

    if (std::holds_alternative<huffman_codeword>(table[key1])) {
      codeword = std::get<huffman_codeword>(table[key1]);
      bs.get_k(codeword.len);

      // std::cerr << "l1 codeword: ";
      // print_binary(std::cerr, key1 >> (8 - codeword.len), codeword.len);
      // std::cerr << '\n';
    }
    else {
      bs.get_k(8);
      int key2 = bs.peak_k(8);

      // std::cerr << "key2: ";
      // print_byte(std::cerr, key2);
      // std::cerr << '\n';

      const huffman_lut& l2 = *(std::get<huffman_supernode>(table[key1]).next);
      codeword = std::get<huffman_codeword>(l2.table[key2]);
      bs.get_k(codeword.len - 8);

      // std::cerr << "l2 codeword: ";
      // print_binary(std::cerr, key1, 8);
      // std::cerr << ' ';
      // print_binary(std::cerr, key2 >> (16 - codeword.len), codeword.len - 8);
      // std::cerr << '\n';
    }

    // std::cerr << "val: ";
    // print_byte(std::cerr, codeword.val);
    // std::cerr << '\n';

    return codeword.val;
  }
};

#endif // HUFFMAN_H
