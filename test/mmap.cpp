#include "mmap.h"
int main(int argc, char** argv) {
  if (argc < 2) {
    return 1;
  }
  std::string filename = argv[1];
  auto p = map_file(filename);
  int fsize = p.first;
  uint8_t* ptr = p.second;
  for (int i = 0; i < 16; i++) {
    std::cerr << std::hex << (int)ptr[i] << '\n';
  }
}
