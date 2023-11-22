#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

#include "huffman.h"
#include "marker.h"
#include "mmap.h"
#include "quantize.h"

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

quant_table qtabs[4];
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

huffman_lut* htabs[2][4];
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

int read_frame(mapped_file mmap, int offset) {
  int size = read_segment_size(mmap, offset + 2);
  int P = mmap[offset + 4];
  int Y = read_u16(mmap, offset + 5);
  int X = read_u16(mmap, offset + 7);
  int Nf = mmap[offset + 9];
  // std::cerr << "P: " << P << '\n';
  // std::cerr << "Y: " << Y << '\n';
  // std::cerr << "X: " << X << '\n';
  // std::cerr << "Nf: " << Nf << '\n';
  int now = 8;
  for (int i = 0; i < Nf; i++) {
    int Ci = mmap[offset + 2 + now];
    now++;
    // std::cerr << "C" << i << ": " << Ci << '\n';
    int tmp = mmap[offset + 2 + now];
    now++;
    int Hi = tmp / 16;
    int Vi = tmp % 16;
    // std::cerr << "H" << i << ", V" << i << ": " << Hi << ", " << Vi << '\n';
    if (Hi < 1 || Hi > 4) {
      throw std::logic_error("bad Hi");
    }
    if (Vi < 1 || Vi > 4) {
      throw std::logic_error("bad Vi");
    }
    int Tqi = mmap[offset + 2 + now];
    now++;
    // std::cerr << "Tq" << i << ": " << Tqi << '\n';
  }
  if (now != size) {
    throw std::logic_error("bad frame header size");
  }
  return 2 + size;
}

int read_scan(mapped_file mmap, int offset) {
  int size = read_segment_size(mmap, offset + 2);
  int now = 2;
  int Ns = mmap[offset + 2 + now];
  now++;
  std::cerr << "Ns: " << Ns << '\n';
  for (int j = 0; j < Ns; j++) {
    int Csj = mmap[offset + 2 + now];
    now++;
    std::cerr << "Cs" << j << ": " << Csj << '\n';
    int tmp = mmap[offset + 2 + now];
    now++;
    int Tdj = tmp / 16;
    int Taj = tmp % 16;
    std::cerr << "Td" << j << ", Ta" << j << ": " << Tdj << ", " << Taj << '\n';
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
  std::cerr << "Ss: " << Ss << '\n';
  std::cerr << "Se: " << Se << '\n';
  int tmp = mmap[offset + 2 + now];
  now++;
  int Ah = tmp / 16;
  int Al = tmp % 16;
  std::cerr << "Ah, Al: " << Ah << ", " << Al << '\n';
  if (now != size) {
    throw std::logic_error("bad scan header size");
  }
  return 2 + size;
}

int parse_segment(mapped_file mmap, int offset) {
  marker_t marker{mmap[offset], mmap[offset + 1]};
  if (marker == SOI) {
    std::cerr << "SOI\n";
    return 2;
  }
  if (marker == EOI) {
    std::cerr << "EOI\n";
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
  std::cerr << std::hex << (int)marker[0] << ' ' << (int)marker[1] << std::dec << '\n';
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
  }

  // std::cout << "hello world\n" << argv[1] << '\n';
  return 0;
}
