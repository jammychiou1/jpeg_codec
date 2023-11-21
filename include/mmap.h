#ifndef MMAP_H
#define MMAP_H

#include <cstdint>
#include <string>
#include <utility>

std::pair<int, uint8_t*> map_file(std::string filename);

#endif // MMAP_H
