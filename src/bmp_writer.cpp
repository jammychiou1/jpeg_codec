#include "bmp_writer.h"

#include <cstdint>
#include <cstring>
#include <string>

void write_u32(mapped_outfile mmap, int offset, uint32_t data) {
  mmap[offset] = data & 0xff;
  mmap[offset + 1] = (data >> 8) & 0xff;
}

void write_u64(mapped_outfile mmap, int offset, uint64_t data) {
  mmap[offset] = data & 0xff;
  mmap[offset + 1] = (data >> 8) & 0xff;
  mmap[offset + 2] = (data >> 16) & 0xff;
  mmap[offset + 3] = (data >> 24) & 0xff;
}

void bmp_writer::new_file(std::string filename, int w, int h) {
  width = w;
  height = h;
  padded_width = ((width * 3 - 1) / 4 + 1) * 4;
  int size = 54 + padded_width * height;
  mmap.new_file(filename, size);
  memset(mmap.ptr, 0, size);
  mmap[0] = 'B';
  mmap[1] = 'M';
  write_u64(mmap, 2, size);
  write_u64(mmap, 10, 54);
  write_u64(mmap, 14, 40);
  write_u64(mmap, 18, width);
  write_u64(mmap, 22, height);
  write_u32(mmap, 26, 1);
  write_u32(mmap, 28, 24);
  write_u64(mmap, 30, 0);
  write_u64(mmap, 34, padded_width * height);
  write_u64(mmap, 38, 2835);
  write_u64(mmap, 42, 2835);
  write_u64(mmap, 46, 0);
  write_u64(mmap, 50, 0);
}
