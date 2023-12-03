#include "bmp_writer.h"

#include <cstdint>
#include <cstring>
#include <string>

void write_u32(uint8_t* ptr, int offset, uint32_t data) {
  ptr[offset] = data & 0xff;
  ptr[offset + 1] = (data >> 8) & 0xff;
}

void write_u64(uint8_t* ptr, int offset, uint64_t data) {
  ptr[offset] = data & 0xff;
  ptr[offset + 1] = (data >> 8) & 0xff;
  ptr[offset + 2] = (data >> 16) & 0xff;
  ptr[offset + 3] = (data >> 24) & 0xff;
}

void bmp_writer::new_file(std::string filename, int h, int w) {
  height = h;
  width = w;
  padded_width = ((width * 3 - 1) / 4 + 1) * 4;
  int size = 54 + padded_width * height;
  mapped_outfile mmap;
  mmap.new_file(filename, size);
  memset(mmap.ptr, 0, size);
  ptr = mmap.ptr;
  ptr[0] = 'B';
  ptr[1] = 'M';
  write_u64(ptr, 2, size);
  write_u64(ptr, 10, 54);
  write_u64(ptr, 14, 40);
  write_u64(ptr, 18, width);
  write_u64(ptr, 22, height);
  write_u32(ptr, 26, 1);
  write_u32(ptr, 28, 24);
  write_u64(ptr, 30, 0);
  write_u64(ptr, 34, padded_width * height);
  write_u64(ptr, 38, 2835);
  write_u64(ptr, 42, 2835);
  write_u64(ptr, 46, 0);
  write_u64(ptr, 50, 0);
}
