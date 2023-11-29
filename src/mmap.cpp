#include "mmap.h"

namespace sys {
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
}

#include <cerrno>
#include <cstdint>
#include <stdexcept>
#include <string>

void mapped_outfile::new_file(std::string filename, int sz) {
  size = sz;
  // TODO save fd
  int fd = sys::open(filename.c_str(), O_RDWR | O_TRUNC | O_CREAT, 0666);
  // int fd = sys::creat(filename.c_str(), 0777);
  if (fd < 0) {
    std::perror("open");
    throw std::logic_error("open file failed");
  }
  if (sys::ftruncate(fd, size)) {
    std::perror("ftruncate");
    throw std::logic_error("resize file failed");
  }
  ptr = (uint8_t*)sys::mmap(nullptr, size, PROT_WRITE, MAP_SHARED, fd, 0);
  if (ptr == MAP_FAILED) {
    std::perror("mmap");
    throw std::runtime_error("file mapping failed");
  }
}

void mapped_file::load_file(std::string filename) {
  // TODO save fd
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
  size = statbuf.st_size;
  ptr = (uint8_t*)sys::mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
  if (ptr == MAP_FAILED) {
    std::perror("mmap");
    throw std::runtime_error("file mapping failed");
  }
}
