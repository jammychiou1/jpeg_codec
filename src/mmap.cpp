#include "mmap.h"

namespace sys {
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
}

#include <cerrno>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

std::pair<int, uint8_t*> map_file(std::string filename) {
  int fd = sys::open(filename.c_str(), O_RDONLY);
  if (fd < 0) {
    std::perror("open");
    throw std::logic_error("open file failed");
  }
  struct sys::stat statbuf;
  if (sys::fstat(fd, &statbuf)) {
    std::perror("fstat");
    throw std::runtime_error("get file size failed");
  }
  int fsize = statbuf.st_size;
  std::cerr << fsize << '\n';
  uint8_t* ptr = (uint8_t*)sys::mmap(nullptr, fsize, PROT_READ, MAP_SHARED, fd, 0);
  if (ptr == MAP_FAILED) {
    std::perror("mmap");
    throw std::runtime_error("file mapping failed");
  }
  return {fsize, ptr};
}
