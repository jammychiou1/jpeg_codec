#include "parser.h"

#include <stdexcept>

#include "decoder.h"
#include "marker.h"
#include "mmap.h"

void setup_frame_info() {
  mapped_file& in_mmap = parser_state.in_mmap;
  marker_segment_t& marker_segment = parser_state.marker_segment;
  frame_info_t& frame = decoder_state.frame_info;
  component_info_t* components = decoder_state.components;

  int offset = marker_segment.offset;
  int size = marker_segment.size;
  int P = in_mmap[offset + 4];
  int Y = read_u16(offset + 5);
  int X = read_u16(offset + 7);
  int Nf = in_mmap[offset + 9];
  frame.precision = P;
  frame.height = Y;
  frame.width = X;
  frame.n_components = Nf;
  std::cerr << "P: " << P << '\n';
  std::cerr << "Y: " << Y << '\n';
  std::cerr << "X: " << X << '\n';
  std::cerr << "Nf: " << Nf << '\n';
  if (Nf <= 0 || Nf > 4) {
    throw std::logic_error("bad component count");
  }
  int now = 8;
  int Hmax = 0;
  int Vmax = 0;
  for (int i = 0; i < Nf; i++) {
    int Ci = in_mmap[offset + 2 + now];
    if (Ci > 4) {
      throw std::runtime_error("large id not supported");
    }
    now++;
    std::cerr << "C" << i << ": " << Ci << '\n';
    int tmp = in_mmap[offset + 2 + now];
    now++;
    int Hi = tmp / 16;
    int Vi = tmp % 16;
    components[Ci].h = Hi;
    components[Ci].v = Vi;
    Hmax = std::max(Hmax, Hi);
    Vmax = std::max(Vmax, Vi);
    std::cerr << "H" << i << ", V" << i << ": " << Hi << ", " << Vi << '\n';
    if (Hi != 1 && Hi != 2 && Hi != 4) {
      throw std::logic_error("bad Hi");
    }
    if (Vi != 1 && Vi != 2 && Vi != 4) {
      throw std::logic_error("bad Vi");
    }
    int Tqi = in_mmap[offset + 2 + now];
    components[Ci].q = Tqi;
    now++;
    std::cerr << "Tq" << i << ": " << Tqi << '\n';
  }
  frame.hmax = Hmax;
  frame.vmax = Vmax;
  if (now != size) {
    throw std::logic_error("bad frame header size");
  }
}

int parse_frame_tbls(int offset) {
  marker_segment_t& marker_segment = parser_state.marker_segment;
  int new_offset = parse_marker_segment(offset);
  if (marker_segment.marker == DHT) {
    setup_huffman_table();
    return new_offset;
  }
  if (marker_segment.marker == DQT) {
    setup_quantization_table();
    return new_offset;
  }
  return offset;
}

int parse_frame_prelude(int offset) {
  int last_offset = offset;
  while (true) {
    offset = parse_misc(offset);
    offset = parse_frame_tbls(offset);
    if (offset == last_offset) {
      return offset;
    }
    last_offset = offset;
  }
}

int parse_frame_hdr(int offset) {
  marker_segment_t& marker_segment = parser_state.marker_segment;
  offset = parse_marker_segment(offset);
  if (marker_segment.marker != SOF0) {
    throw std::logic_error("expecting frame header");
  }
  setup_frame_info();
  return offset;
}

int parse_frame(int offset) {
  offset = parse_frame_prelude(offset);
  offset = parse_frame_hdr(offset);
  offset = parse_scan(offset);
  return offset;
}
