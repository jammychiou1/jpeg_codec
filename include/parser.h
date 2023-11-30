#ifndef PARSER_H
#define PARSER_H

#include <string>

#include "marker.h"
#include "mmap.h"

struct parser_state_t {
  mapped_file in_mmap;
  marker_segment_t marker_segment;
};

extern parser_state_t parser_state;

void load_file(std::string filename);

static int read_u16(int offset) {
  mapped_file& in_mmap = parser_state.in_mmap;
  return (int(in_mmap[offset]) << 8) | in_mmap[offset + 1];
}

int parse_marker_segment(int offset);
int parse_misc(int offset);
void setup_huffman_table();
void setup_quantization_table();

int parse_frame(int offset);
int parse_scan(int offset);

#endif // PARSER_H
