#include "parser.h"

#include <cstdint>
#include <stdexcept>

#include "decoder.h"
#include "marker.h"
#include "mmap.h"

void setup_scan_info() {
  mapped_file& in_mmap = parser_state.in_mmap;
  marker_segment_t& marker_segment = parser_state.marker_segment;
  scan_info_t& scan = decoder_state.scan_info;

  int offset = marker_segment.offset;
  int size = marker_segment.size;
  int now = 2;
  int Ns = in_mmap[offset + 2 + now];
  now++;
  scan.n_components = Ns;
  std::cerr << "Ns: " << Ns << '\n';
  for (int j = 0; j < Ns; j++) {
    int Csj = in_mmap[offset + 2 + now];
    now++;
    std::cerr << "Cs" << j << ": " << Csj << '\n';
    scan.ids[j] = Csj;
    int tmp = in_mmap[offset + 2 + now];
    now++;
    int Tdj = tmp / 16;
    scan.tds[j] = Tdj;
    int Taj = tmp % 16;
    scan.tas[j] = Taj;
    std::cerr << "Td" << j << ", Ta" << j << ": " << Tdj << ", " << Taj << '\n';
    if (Tdj < 0 || Tdj > 3) {
      throw std::logic_error("bad Tdj");
    }
    if (Taj < 0 || Taj > 3) {
      throw std::logic_error("bad Taj");
    }
  }
  int Ss = in_mmap[offset + 2 + now];
  now++;
  int Se = in_mmap[offset + 2 + now];
  now++;
  std::cerr << "Ss: " << Ss << '\n';
  std::cerr << "Se: " << Se << '\n';
  int tmp = in_mmap[offset + 2 + now];
  now++;
  int Ah = tmp / 16;
  int Al = tmp % 16;
  std::cerr << "Ah, Al: " << Ah << ", " << Al << '\n';
  if (now != size) {
    throw std::logic_error("bad scan header size");
  }
}

int parse_scan_tbls(int offset) {
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

int parse_scan_prelude(int offset) {
  marker_segment_t& marker_segment = parser_state.marker_segment;
  int last_offset = offset;
  while (true) {
    offset = parse_misc(offset);
    offset = parse_scan_tbls(offset);
    if (offset == last_offset) {
      return offset;
    }
    last_offset = offset;
  }
}

int parse_scan_hdr(int offset) {
  marker_segment_t& marker_segment = parser_state.marker_segment;
  offset = parse_marker_segment(offset);
  if (marker_segment.marker != SOS) {
    throw std::logic_error("expecting scan header");
  }
  setup_scan_info();
  return offset;
}

int parse_scan_body(int offset) {
  mapped_file& in_mmap = parser_state.in_mmap;
  marker_t next_marker;
  int left = 0;
  int right = 0;
  bool finish = false;
  while (!finish) {
    right = left;
    while (true) {
      if (in_mmap[offset + right] != 0xff) {
        right++;
        continue;
      }
      if (in_mmap[offset + right + 1] == 0x00) {
        right += 2;
        continue;
      }
      next_marker = marker_t{in_mmap[offset + right], in_mmap[offset + right + 1]};
      break;
    }

    // decode_scan_segment(in_mmap, offset + left, offset + right);

    finish = true;
    for (int i = 0; i < 8; i++) {
      if (next_marker == RST[i]) {
        finish = false;
        left = right + 2;
        break;
      }
    }
  }
  return offset + right;
}

int parse_scan(int offset) {
  offset = parse_scan_prelude(offset);
  offset = parse_scan_hdr(offset);
  offset = parse_scan_body(offset);
  return offset;
}

