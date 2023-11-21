#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

#include "quantize.h"
#include "huffman.h"

typedef std::array<uint8_t, 2> marker_t;

int get_u8(std::ifstream& file) {
  std::array<uint8_t, 1> data;
  if (!file.read((char*)&data[0], 1)) {
    throw std::logic_error("read int fail");
  }
  return data[0];
}
int get_u16(std::ifstream& file) {
  std::array<uint8_t, 2> data;
  if (!file.read((char*)&data[0], 2)) {
    throw std::logic_error("read int fail");
  }
  return data[0] * 0x100 + data[1];
}

quant_table qtabs[4];
void get_quantization_table(std::ifstream& file) {
  int size = get_u16(file);
  // std::cerr << "size " << size << '\n';
  if (size < 2) {
    throw std::logic_error("bad payload size");
  }
  int now = 2;
  while (now < size) {
    int tmp = get_u8(file);
    int Pq = tmp / 16;
    int Tq = tmp % 16;
    // std::cerr << "Pq, Tq: " << Pq << ", " << Tq << '\n';
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
        Qk = get_u8(file);
      }
      else {
        Qk = get_u16(file);
      }
      qtabs[Tq].table[k] = Qk;
      // std::cerr << "Q" << k << ": " << Qk << '\n';
    }
    now += 65 + Pq * 64;
  }
  if (now != size) {
    throw std::logic_error("bad table size");
  }
}

huffman_lut* htabs[2][4];
void get_huffman_table(std::ifstream& file) {
  int size = get_u16(file);
  // std::cerr << "size " << size << '\n';
  if (size < 2) {
    throw std::logic_error("bad payload size");
  }
  int now = 2;
  while (now < size) {
    int tmp = get_u8(file);
    int Tc = tmp / 16;
    int Th = tmp % 16;
    std::cerr << "Tc, Th: " << Tc << ", " << Th << '\n';
    if (Tc < 0 || Tc > 1) {
      throw std::logic_error("bad Tc");
    }
    if (Th < 0 || Th > 3) {
      throw std::logic_error("bad Th");
    }
    htabs[Tc][Th] = new huffman_lut();
    std::array<int, 16> Li_s;
    for (int i = 0; i < 16; i++) {
      Li_s[i] = get_u8(file);
      std::cerr << "L" << i << ": " << Li_s[i] << '\n';
    }
    now += 17;
    int code = 0;
    for (int i = 0; i < 16; i++) {
      for (int j = 0; j < Li_s[i]; j++) {
        int Vij = get_u8(file);
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

void get_frame(std::ifstream& file) {
  int size = get_u16(file);
  // std::cerr << "size " << size << '\n';
  if (size < 2) {
    throw std::logic_error("bad payload size");
  }
  int P = get_u8(file);
  int Y = get_u16(file);
  int X = get_u16(file);
  int Nf = get_u8(file);
  // std::cerr << "P: " << P << '\n';
  // std::cerr << "Y: " << Y << '\n';
  // std::cerr << "X: " << X << '\n';
  // std::cerr << "Nf: " << Nf << '\n';
  for (int i = 0; i < Nf; i++) {
    int Ci = get_u8(file);
    // std::cerr << "C" << i << ": " << Ci << '\n';
    int tmp = get_u8(file);
    int Hi = tmp / 16;
    int Vi = tmp % 16;
    // std::cerr << "H" << i << ", V" << i << ": " << Hi << ", " << Vi << '\n';
    if (Hi < 1 || Hi > 4) {
      throw std::logic_error("bad Hi");
    }
    if (Vi < 1 || Vi > 4) {
      throw std::logic_error("bad Vi");
    }
    int Tqi = get_u8(file);
    // std::cerr << "Tq" << i << ": " << Tqi << '\n';
  }
}

void get_scan(std::ifstream& file) {
  int size = get_u16(file);
  // std::cerr << "size " << size << '\n';
  if (size < 2) {
    throw std::logic_error("bad payload size");
  }
  int Ns = get_u8(file);
  std::cerr << "Ns: " << Ns << '\n';
  for (int j = 0; j < Ns; j++) {
    int Csj = get_u8(file);
    std::cerr << "Cs" << j << ": " << Csj << '\n';
    int tmp = get_u8(file);
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
  int Ss = get_u8(file);
  int Se = get_u8(file);
  std::cerr << "Ss: " << Ss << '\n';
  std::cerr << "Se: " << Se << '\n';
  int tmp = get_u8(file);
  int Ah = tmp / 16;
  int Al = tmp % 16;
  std::cerr << "Ah, Al: " << Ah << ", " << Al << '\n';
}

void get_segment(std::ifstream& file) {
  marker_t marker;
  if (!file.read((char*)&marker[0], 2)) {
    throw std::logic_error("read marker fail");
  }
  if (marker[0] != 0xff) {
    std::cerr << std::hex << (int)marker[0] << ' ' << (int)marker[1] << std::dec << '\n';
    throw std::logic_error("unknown marker");
  }
  if (marker == marker_t{0xff, 0xd8}) {
    std::cerr << "SOI\n";
    return;
  }
  if (marker == marker_t{0xff, 0xd9}) {
    std::cerr << "EOI\n";
    return;
  }
  if (marker == marker_t{0xff, 0xfe}) {
    std::cerr << "COM\n";
    int size = get_u16(file);
    // std::cerr << "size " << size << '\n';
    if (size < 2) {
      throw std::logic_error("bad payload size");
    }
    std::string comment(size, '\0');
    if (!file.read((char*)&comment[0], size - 2)) {
      throw std::logic_error("read payload fail");
    }
    // std::cerr << comment << '\n';
    return;
  }
  if (marker == marker_t{0xff, 0xc0}) {
    std::cerr << "SOF0\n";
    get_frame(file);
    return;
  }
  for (int i = 0; i < 16; i++) {
    if (marker == marker_t{0xff, uint8_t(0xe0 + i)}) {
      std::cerr << "APP" << i << '\n';
      int size = get_u16(file);
      // std::cerr << "size " << size << '\n';
      if (size < 2) {
        throw std::logic_error("bad payload size");
      }
      if (!file.ignore(size - 2)) {
        throw std::logic_error("read payload fail");
      }
      return;
    }
  }
  if (marker == marker_t{0xff, 0xc4}) {
    std::cerr << "DHT\n";
    get_huffman_table(file);
    return;
  }
  if (marker == marker_t{0xff, 0xdb}) {
    std::cerr << "DQT\n";
    get_quantization_table(file);
    return;
  }
  if (marker == marker_t{0xff, 0xda}) {
    std::cerr << "SOS\n";
    get_scan(file);
    return;
  }
}

int main(int argc, char** argv) {
  if (argc < 2) {
    return 1;
  }
  std::string filename = argv[1];
  std::ifstream file(filename, std::ios::binary);
  while (true) {
    get_segment(file);
  }

  // std::cout << "hello world\n" << argv[1] << '\n';
  return 0;
}
