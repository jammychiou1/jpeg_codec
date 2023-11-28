#include <array>
#include <cstdint>
#include <fstream>
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

quant_table qtabs[4];
huffman_lut* htabs[2][4];

int read_u16(mapped_file mmap, int offset) {
  return (((int)mmap[offset]) << 8) | mmap[offset + 1];
}

int read_segment_size(mapped_file mmap, int offset) {
  int size = read_u16(mmap, offset);
  if (size < 2) {
    throw std::logic_error("bad payload size");
  }
  return size;
}

int read_quantization_table(mapped_file mmap, int offset) {
  int size = read_segment_size(mmap, offset + 2);
  int now = 2;
  while (now < size) {
    int tmp = mmap[offset + 2 + now];
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
        Qk = mmap[offset + 2 + now];
        now++;
      }
      else {
        Qk = read_u16(mmap, offset + 2 + now);
        now += 2;
      }
      qtabs[Tq].table[k] = Qk;
      // std::cerr << "Q" << k << ": " << Qk << '\n';
    }
  }
  if (now != size) {
    throw std::logic_error("bad table size");
  }
  return 2 + size;
}

int read_huffman_table(mapped_file mmap, int offset) {
  int size = read_segment_size(mmap, offset + 2);
  int now = 2;
  while (now < size) {
    int tmp = mmap[offset + 2 + now];
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
    htabs[Tc][Th] = new huffman_lut();

    std::array<int, 16> Li_s;
    for (int i = 0; i < 16; i++) {
      Li_s[i] = mmap[offset + 2 + now];
      // std::cerr << "L" << i << ": " << Li_s[i] << '\n';
      now++;
    }

    int code = 0;
    for (int i = 0; i < 16; i++) {
      for (int j = 0; j < Li_s[i]; j++) {
        int Vij = mmap[offset + 2 + now];
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
  return 2 + size;
}

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

frame_info_t frame;

int read_frame(mapped_file mmap, int offset) {
  int size = read_segment_size(mmap, offset + 2);
  int P = mmap[offset + 4];
  int Y = read_u16(mmap, offset + 5);
  int X = read_u16(mmap, offset + 7);
  int Nf = mmap[offset + 9];
  frame.precision = P;
  frame.height = Y;
  frame.width = X;
  frame.n_components = Nf;
  // std::cerr << "P: " << P << '\n';
  std::cerr << "Y: " << Y << '\n';
  std::cerr << "X: " << X << '\n';
  // std::cerr << "Nf: " << Nf << '\n';
  if (Nf <= 0 || Nf > 4) {
    throw std::logic_error("bad component count");
  }
  int now = 8;
  int Hmax = 0;
  int Vmax = 0;
  for (int i = 0; i < Nf; i++) {
    int Ci = mmap[offset + 2 + now];
    if (Ci > 4) {
      throw std::runtime_error("large id not supported");
    }
    now++;
    std::cerr << "C" << i << ": " << Ci << '\n';
    int tmp = mmap[offset + 2 + now];
    now++;
    int Hi = tmp / 16;
    int Vi = tmp % 16;
    frame.components[Ci].h = Hi;
    frame.components[Ci].v = Vi;
    Hmax = std::max(Hmax, Hi);
    Vmax = std::max(Vmax, Vi);
    std::cerr << "H" << i << ", V" << i << ": " << Hi << ", " << Vi << '\n';
    if (Hi != 1 && Hi != 2 && Hi != 4) {
      throw std::logic_error("bad Hi");
    }
    if (Vi != 1 && Vi != 2 && Vi != 4) {
      throw std::logic_error("bad Vi");
    }
    int Tqi = mmap[offset + 2 + now];
    frame.components[Ci].q = Tqi;
    now++;
    // std::cerr << "Tq" << i << ": " << Tqi << '\n';
  }
  frame.hmax = Hmax;
  frame.vmax = Vmax;
  if (now != size) {
    throw std::logic_error("bad frame header size");
  }
  return 2 + size;
}

struct scan_info_t {
  int n_components;
  std::array<int, 4> ids;
  std::array<int, 4> tds;
  std::array<int, 4> tas;
};

scan_info_t scan;

int decode_coef(int len, int bits) {
  if (len == 0) {
    return 0;
  }
  if (bits & (1 << (len - 1))) {
    return bits;
  }
  int low = bits & low_k_mask(len - 1);
  return 1 - (1 << len) + low;
}

void decode_block(unstuffing_bitstream& bs, const huffman_lut& lut_dc, const huffman_lut& lut_ac) {
  // std::cerr << "DC:\n";
  {
    int cate = lut_dc.lookup(bs);
    int diff = decode_coef(cate, bs.get_k(cate));
    // std::cerr << "  " << diff << '\n';
  }

  // std::cerr << "AC:\n";
  {
    // std::cerr << " lut @ " << (void*)&lut << '\n';
    for (int j = 1; j < 64; j++) {
      int tmp = lut_ac.lookup(bs);
      if (tmp == 0x00) {
        // std::cerr << "(EOB)\n";
        break;
      }
      int run = tmp / 16;
      j += run;
      int cate = tmp % 16;
      int diff = decode_coef(cate, bs.get_k(cate));

      // std::cerr << "  " << j << "  " << diff << '\n';
    }
  }
}

void decode_scan_segment(mapped_file mmap, int left, int right) {
  int num_mcu = 0;
  int du_per_mcu = 0;
  if (scan.n_components == 1) {
    int cols = (frame.width - 1) / 8 + 1;
    int rows = (frame.height - 1) / 8 + 1;
    num_mcu = cols * rows;
    du_per_mcu = 1;
  }
  else {
    int cols = (frame.width - 1) / (frame.hmax * 8) + 1;
    int rows = (frame.height - 1) / (frame.vmax * 8) + 1;
    num_mcu = cols * rows;
    for (int j = 0; j < scan.n_components; j++) {
      component_info_t component = frame.components[scan.ids[j]];
      du_per_mcu += component.h * component.v;
    }
  }
  std::cerr << "expecting " << num_mcu << " MCUs, " << du_per_mcu << " DUs per MCU\n";

  unstuffing_bitstream bs;
  bs.set_mmap(mmap, left, right);
  int now = left;
  for (int t = 0; t < num_mcu; t++) {
    std::cerr << "MCU " << t << '\n';
    if (scan.n_components == 1) {
      decode_block(bs, *htabs[0][scan.tds[0]], *htabs[1][scan.tas[0]]);
    }
    else {
      for (int j = 0; j < scan.n_components; j++) {
        std::cerr << "component id: " << scan.ids[j] << '\n';

        component_info_t component = frame.components[scan.ids[j]];
        for (int y = 0; y < component.v; y++) {
          for (int x = 0; x < component.h; x++) {
            std::cerr << "DU " << x << ' ' << y << '\n';

            decode_block(bs, *htabs[0][scan.tds[j]], *htabs[1][scan.tas[j]]);
          }
        }
      }
    }
  }
}

int read_scan_segments(mapped_file mmap, int offset) {
  marker_t next_marker;
  int left = 0;
  int right = 0;
  bool finish = false;
  while (!finish) {
    right = left;
    while (true) {
      if (mmap[offset + right] != 0xff) {
        right++;
        continue;
      }
      if (mmap[offset + right + 1] == 0x00) {
        right += 2;
        continue;
      }
      next_marker = marker_t{mmap[offset + right], mmap[offset + right + 1]};
      // print_byte(std::cerr, next_marker[0]);
      // std::cerr << ' ';
      // print_byte(std::cerr, next_marker[1]);
      // std::cerr << '\n';
      break;
    }

    // for (int i = left; i < std::min(left + 10, right); i++) {
    //   print_byte(std::cerr, mmap[offset + i]);
    //   if (mmap[offset + i] == 0xff) {
    //     i++;
    //   }
    // }
    // std::cerr << '\n';

    decode_scan_segment(mmap, offset + left, offset + right);

    finish = true;
    for (int i = 0; i < 8; i++) {
      if (next_marker == RST[i]) {
        finish = false;
        left = right + 2;
        break;
      }
    }
  }
  return right;
}

int read_scan(mapped_file mmap, int offset) {
  int size = read_segment_size(mmap, offset + 2);
  int now = 2;
  int Ns = mmap[offset + 2 + now];
  now++;
  scan.n_components = Ns;
  // std::cerr << "Ns: " << Ns << '\n';
  for (int j = 0; j < Ns; j++) {
    int Csj = mmap[offset + 2 + now];
    now++;
    // std::cerr << "Cs" << j << ": " << Csj << '\n';
    scan.ids[j] = Csj;
    int tmp = mmap[offset + 2 + now];
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
  int Ss = mmap[offset + 2 + now];
  now++;
  int Se = mmap[offset + 2 + now];
  now++;
  // std::cerr << "Ss: " << Ss << '\n';
  // std::cerr << "Se: " << Se << '\n';
  int tmp = mmap[offset + 2 + now];
  now++;
  int Ah = tmp / 16;
  int Al = tmp % 16;
  // std::cerr << "Ah, Al: " << Ah << ", " << Al << '\n';
  if (now != size) {
    throw std::logic_error("bad scan header size");
  }
  return 2 + size + read_scan_segments(mmap, offset + 2 + size);
}

bool parse_segment_done = false;
int parse_segment(mapped_file mmap, int offset) {
  marker_t marker{mmap[offset], mmap[offset + 1]};
  if (marker == SOI) {
    std::cerr << "SOI\n";
    return 2;
  }
  if (marker == EOI) {
    std::cerr << "EOI\n";
    parse_segment_done = true;
    return 2;
  }
  if (marker == COM) {
    std::cerr << "COM\n";
    int size = read_segment_size(mmap, offset + 2);
    return size + 2;
  }
  if (marker == SOF0) {
    std::cerr << "SOF0\n";
    return read_frame(mmap, offset);
  }
  for (int i = 0; i < 16; i++) {
    if (marker == APP[i]) {
      std::cerr << "APP" << i << '\n';
      int size = read_segment_size(mmap, offset + 2);
      return size + 2;
    }
  }
  if (marker == DRI) {
    std::cerr << "DRI\n";
    int size = read_segment_size(mmap, offset + 2);
    return size + 2;
  }
  if (marker == DHT) {
    std::cerr << "DHT\n";
    return read_huffman_table(mmap, offset);
  }
  if (marker == DQT) {
    std::cerr << "DQT\n";
    return read_quantization_table(mmap, offset);
  }
  if (marker == SOS) {
    std::cerr << "SOS\n";
    return read_scan(mmap, offset);
  }
  print_byte(std::cerr, marker[0]);
  std::cerr << ' ';
  print_byte(std::cerr, marker[1]);
  std::cerr << '\n';
  throw std::logic_error("unknown marker");
}

int main(int argc, char** argv) {
  if (argc < 2) {
    return 1;
  }
  std::string filename = argv[1];
  mapped_file mmap;
  mmap.load_file(filename);

  int now = 0;
  while (now < mmap.size) {
    now += parse_segment(mmap, now);
    std::cerr << now << '\n';
    if (parse_segment_done) {
      return 0;
    }
  }
  return 1;
}
