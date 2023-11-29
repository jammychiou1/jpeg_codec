#ifndef MMAP_H
#define MMAP_H

#include <cstdint>
#include <string>
#include <stdexcept>

struct mapped_file {
    const uint8_t* ptr;
    int size;
    uint8_t operator[](int offset) {
      if (offset < 0 || offset >= size) {
        throw std::logic_error("bad index");
      }
      return ptr[offset];
    }
    void load_file(std::string filename);
};

struct mapped_outfile {
    uint8_t* ptr;
    int size;
    uint8_t& operator[](int offset) {
      if (offset < 0 || offset >= size) {
        throw std::logic_error("bad index");
      }
      return ptr[offset];
    }
    void new_file(std::string filename, int sz);
};

#endif // MMAP_H
