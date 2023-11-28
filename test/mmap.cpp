#include <iostream>
#include <string>
#include <iomanip>
#include "mmap.h"
int main(int argc, char** argv) {
  if (argc < 2) {
    return 1;
  }
  std::string filename = argv[1];
  mapped_file mmap;
  mmap.load_file(filename);
  for (int i = 0; i < 16; i++) {
    std::cerr << std::hex << (int)mmap[i] << '\n';
  }
}
