#ifndef BMP_WRITER_H
#define BMP_WRITER_H

#include "mmap.h"

struct bmp_writer {
  int width;
  int height;
  int padded_width;
  mapped_outfile mmap;
  void new_file(std::string filename, int width, int height);
};

#endif // BMP_WRITER_H
