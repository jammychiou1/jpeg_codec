#include <string>

#include "decoder.h"
#include "mmap.h"
#include "parser.h"

parser_state_t parser;
decoder_state_t decoder;
int main(int argc, char** argv) {
  if (argc < 2) {
    return 1;
  }
  std::string filename = argv[1];
  mapped_file mmap;
  mmap.load_file(filename);
  parser.ptr = mmap.ptr;
  parser.size = mmap.size;
  parser.dcd = &decoder;
  parse_image(parser);
}
