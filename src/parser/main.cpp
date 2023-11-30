#include "parser.h"

#include <stdexcept>
#include <string>

#include "decoder.h"
#include "entropy_coded.h"
#include "huffman.h"
#include "marker.h"
#include "mmap.h"
#include "quantize.h"
#include "util.h"

parser_state_t parser_state;

void setup_huffman_table() {
  mapped_file& in_mmap = parser_state.in_mmap;
  marker_segment_t& marker_segment = parser_state.marker_segment;
  int offset = marker_segment.offset;
  int size = marker_segment.size;
  int now = 2;
  while (now < size) {
    int tmp = in_mmap[offset + 2 + now];
    int Tc = tmp / 16;
    int Th = tmp % 16;
    std::cerr << "Tc, Th: " << Tc << ", " << Th << '\n';
    now++;
    if (Tc < 0 || Tc > 1) {
      throw std::logic_error("bad Tc");
    }
    if (Th < 0 || Th > 3) {
      throw std::logic_error("bad Th");
    }
    // TODO properly free
    decoder_state.htabs[Tc][Th] = new huffman_lut();

    int Li_s[16];
    for (int i = 0; i < 16; i++) {
      Li_s[i] = in_mmap[offset + 2 + now];
      std::cerr << "L" << i << ": " << Li_s[i] << '\n';
      now++;
    }

    int code = 0;
    for (int i = 0; i < 16; i++) {
      for (int j = 0; j < Li_s[i]; j++) {
        int Vij = in_mmap[offset + 2 + now];
        decoder_state.htabs[Tc][Th]->add_codeword(code, i + 1, Vij);
        now++;
        code++;
      }
      code *= 2;
    }
  }
  if (now != size) {
    throw std::logic_error("bad table size");
  }
}

void setup_quantization_table() {
  mapped_file& in_mmap = parser_state.in_mmap;
  marker_segment_t& marker_segment = parser_state.marker_segment;
  int offset = marker_segment.offset;
  int size = marker_segment.size;
  int now = 2;
  while (now < size) {
    int tmp = in_mmap[offset + 2 + now];
    int Pq = tmp / 16;
    int Tq = tmp % 16;
    std::cerr << "Pq, Tq: " << Pq << ", " << Tq << '\n';
    now++;
    if (Pq < 0 || Pq > 1) {
      throw std::logic_error("bad Pq");
    }
    if (Tq < 0 || Tq > 3) {
      throw std::logic_error("bad Tq");
    }
    decoder_state.qtabs[Tq].precision = Pq;
    for (int k = 0; k < 64; k++) {
      int Qk;
      if (Pq == 0) {
        Qk = in_mmap[offset + 2 + now];
        now++;
      }
      else {
        Qk = read_u16(offset + 2 + now);
        now += 2;
      }
      decoder_state.qtabs[Tq].table[k] = Qk;
      std::cerr << "Q" << k << ": " << Qk << '\n';
    }
  }
  if (now != size) {
    throw std::logic_error("bad table size");
  }
}

int parse_marker_segment(int offset) {
  mapped_file& in_mmap = parser_state.in_mmap;
  marker_segment_t& marker_segment = parser_state.marker_segment;
  // std::cerr << offset << '\n';
  if (in_mmap[offset] != 0xff) {
    throw std::logic_error("expecting marker");
  }
  marker_segment.offset = offset;
  marker_segment.marker = {in_mmap[offset], in_mmap[offset + 1]};
  offset += 2;
  if (variable_size_mrksegs.count(marker_segment.marker)) {
    int size = read_u16(offset);
    if (size < 2) {
      throw std::logic_error("bad marker segment size");
    }
    marker_segment.size = size;
    offset += size;
  }
  return offset;
}

int parse_misc(int offset) {
  marker_segment_t& marker_segment = parser_state.marker_segment;
  int new_offset = parse_marker_segment(offset);
  for (int i = 0; i < 16; i++) {
    if (marker_segment.marker == APP[i]) {
      return new_offset;
    }
  }
  if (marker_segment.marker == DRI) {
    return new_offset;
  }
  if (marker_segment.marker == COM) {
    return new_offset;
  }
  return offset;
}

int parse_image() {
  marker_segment_t& marker_segment = parser_state.marker_segment;
  int offset = 0;
  offset = parse_marker_segment(offset);
  if (marker_segment.marker != SOI) {
    throw std::logic_error("expecting SOI");
  }
  // std::cerr << "SOI\n";
  offset = parse_frame(offset);
  offset = parse_marker_segment(offset);
  if (marker_segment.marker != EOI) {
    throw std::logic_error("expecting EOI");
  }
  // std::cerr << "EOI\n";
  return offset;
}

void load_file(std::string filename) {
  parser_state.in_mmap.load_file(filename);
  parse_image();
}
