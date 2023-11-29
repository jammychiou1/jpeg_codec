#include "parser.h"

#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

#include "entropy_coded.h"
#include "huffman.h"
#include "marker.h"
#include "mmap.h"
#include "quantize.h"
#include "util.h"

mapped_file in_mmap;

marker_segment_t marker_segment;

struct component_info_t {
  int h;
  int v;
  int q;
};

struct frame_info_t {
  int precision;
  int height;
  int width;
  int n_components;
  int hmax;
  int vmax;
  std::array<component_info_t, 4> components;
};

struct scan_info_t {
  int n_components;
  std::array<int, 4> ids;
  std::array<int, 4> tds;
  std::array<int, 4> tas;
};

frame_info_t frame;
scan_info_t scan;
quant_table qtabs[4];
huffman_lut* htabs[2][4];

int read_u16(int offset) {
  return (int(in_mmap[offset]) << 8) | in_mmap[offset + 1];
}

void setup_frame_info() {
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
  // std::cerr << "P: " << P << '\n';
  // std::cerr << "Y: " << Y << '\n';
  // std::cerr << "X: " << X << '\n';
  // std::cerr << "Nf: " << Nf << '\n';
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
    // std::cerr << "C" << i << ": " << Ci << '\n';
    int tmp = in_mmap[offset + 2 + now];
    now++;
    int Hi = tmp / 16;
    int Vi = tmp % 16;
    frame.components[Ci].h = Hi;
    frame.components[Ci].v = Vi;
    Hmax = std::max(Hmax, Hi);
    Vmax = std::max(Vmax, Vi);
    // std::cerr << "H" << i << ", V" << i << ": " << Hi << ", " << Vi << '\n';
    if (Hi != 1 && Hi != 2 && Hi != 4) {
      throw std::logic_error("bad Hi");
    }
    if (Vi != 1 && Vi != 2 && Vi != 4) {
      throw std::logic_error("bad Vi");
    }
    int Tqi = in_mmap[offset + 2 + now];
    frame.components[Ci].q = Tqi;
    now++;
    // std::cerr << "Tq" << i << ": " << Tqi << '\n';
  }
  frame.hmax = Hmax;
  frame.vmax = Vmax;
  if (now != size) {
    throw std::logic_error("bad frame header size");
  }
}

void setup_scan_info() {
  int offset = marker_segment.offset;
  int size = marker_segment.size;
  int now = 2;
  int Ns = in_mmap[offset + 2 + now];
  now++;
  scan.n_components = Ns;
  // std::cerr << "Ns: " << Ns << '\n';
  for (int j = 0; j < Ns; j++) {
    int Csj = in_mmap[offset + 2 + now];
    now++;
    // std::cerr << "Cs" << j << ": " << Csj << '\n';
    scan.ids[j] = Csj;
    int tmp = in_mmap[offset + 2 + now];
    now++;
    int Tdj = tmp / 16;
    scan.tds[j] = Tdj;
    int Taj = tmp % 16;
    scan.tas[j] = Taj;
    // std::cerr << "Td" << j << ", Ta" << j << ": " << Tdj << ", " << Taj << '\n';
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
  // std::cerr << "Ss: " << Ss << '\n';
  // std::cerr << "Se: " << Se << '\n';
  int tmp = in_mmap[offset + 2 + now];
  now++;
  int Ah = tmp / 16;
  int Al = tmp % 16;
  // std::cerr << "Ah, Al: " << Ah << ", " << Al << '\n';
  if (now != size) {
    throw std::logic_error("bad scan header size");
  }
}

void setup_huffman_table() {
  int offset = marker_segment.offset;
  int size = marker_segment.size;
  int now = 2;
  while (now < size) {
    int tmp = in_mmap[offset + 2 + now];
    int Tc = tmp / 16;
    int Th = tmp % 16;
    // std::cerr << "Tc, Th: " << Tc << ", " << Th << '\n';
    now++;
    if (Tc < 0 || Tc > 1) {
      throw std::logic_error("bad Tc");
    }
    if (Th < 0 || Th > 3) {
      throw std::logic_error("bad Th");
    }
    // TODO properly free
    htabs[Tc][Th] = new huffman_lut();

    std::array<int, 16> Li_s;
    for (int i = 0; i < 16; i++) {
      Li_s[i] = in_mmap[offset + 2 + now];
      // std::cerr << "L" << i << ": " << Li_s[i] << '\n';
      now++;
    }

    int code = 0;
    for (int i = 0; i < 16; i++) {
      for (int j = 0; j < Li_s[i]; j++) {
        int Vij = in_mmap[offset + 2 + now];
        htabs[Tc][Th]->add_codeword(code, i + 1, Vij);
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
  int offset = marker_segment.offset;
  int size = marker_segment.size;
  int now = 2;
  while (now < size) {
    int tmp = in_mmap[offset + 2 + now];
    int Pq = tmp / 16;
    int Tq = tmp % 16;
    // std::cerr << "Pq, Tq: " << Pq << ", " << Tq << '\n';
    now++;
    if (Pq < 0 || Pq > 1) {
      throw std::logic_error("bad Pq");
    }
    if (Tq < 0 || Tq > 3) {
      throw std::logic_error("bad Tq");
    }
    qtabs[Tq].precision = Pq;
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
      qtabs[Tq].table[k] = Qk;
      // std::cerr << "Q" << k << ": " << Qk << '\n';
    }
  }
  if (now != size) {
    throw std::logic_error("bad table size");
  }
}

int parse_marker_segment(int offset) {
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

int parse_scan_tbls(int offset) {
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
  offset = parse_marker_segment(offset);
  if (marker_segment.marker != SOS) {
    throw std::logic_error("expecting scan header");
  }
  setup_scan_info();
  return offset;
}

int parse_scan_body(int offset) {
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

int parse_frame_tbls(int offset) {
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

int parse_image() {
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
  in_mmap.load_file(filename);
  parse_image();
}
