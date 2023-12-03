#ifndef HUFFMAN_H
#define HUFFMAN_H

struct huffman_lut;

struct huffman_codeword {
  int len;
  int val;
};

struct huffman_entry {
  bool is_cw;
  union {
    huffman_lut* next;
    huffman_codeword cw;
  } u;
};

struct huffman_lut {
  huffman_entry entries[256];
  void add_codeword(int key, int len, int val);
};

#endif // HUFFMAN_H
