#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "bmp_writer.h"
#include "decoder.h"
#include "mmap.h"
#include "parser.h"

using std::string, std::vector, std::array;
using std::max, std::min, std::round;
using std::ofstream;

parser_state_t parser;
decoder_state_t decoder;

void save_dbg(int height, int width, string dbg_filename, vector<uint8_t>& y_data, vector<uint8_t>& cb_data, vector<uint8_t>& cr_data) {
  ofstream file(dbg_filename);
  file << height << ' ' << width << '\n';
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      file << int(y_data[i * width + j]) << " \n"[j == width - 1];
    }
  }
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      file << int(cb_data[i * width + j]) << " \n"[j == width - 1];
    }
  }
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      file << int(cr_data[i * width + j]) << " \n"[j == width - 1];
    }
  }
}

void save_bmp(int height, int width, string bmp_filename, vector<uint8_t>& y_data, vector<uint8_t>& cb_data, vector<uint8_t>& cr_data) {
  bmp_writer wrt;
  wrt.new_file(bmp_filename, height, width);

  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      double y = y_data[i * width + j];
      double cb = cb_data[i * width + j];
      double cr = cr_data[i * width + j];

      int r = round(y + 1.402 * (cr - 128));
      int g = round(y - 0.344136 * (cb - 128) - 0.714136 * (cr - 128));
      int b = round(y + 1.402 * (cb - 128));
      r = max(0, min(255, r));
      g = max(0, min(255, g));
      b = max(0, min(255, b));

      int begin = 54 + (height - 1 - i) * wrt.padded_width + j * 3;
      wrt.ptr[begin + 2] = r;
      wrt.ptr[begin + 1] = g;
      wrt.ptr[begin + 0] = b;
    }
  }
}

int main(int argc, char** argv) {
  string jpg_filename;
  string bmp_filename;
  string dbg_filename;
  for (int i = 1; i < argc; i++) {
    string arg = argv[i];
    if (arg == "-o" && i + 1 < argc) {
      bmp_filename = argv[i + 1];
      i++;
    }
    else if (arg == "-d" && i + 1 < argc) {
      dbg_filename = argv[i + 1];
      i++;
    }
    else {
      jpg_filename = argv[i];
    }
  }
  if (jpg_filename == "") {
    return 1;
  }

  mapped_file mmap;
  mmap.load_file(jpg_filename);
  parser.ptr = mmap.ptr;
  parser.size = mmap.size;
  parser.dcd = &decoder;
  parse_image(parser);

  if (dbg_filename != "") {
    save_dbg(decoder.frame.y, decoder.frame.x, dbg_filename, decoder.pixels[1], decoder.pixels[2], decoder.pixels[3]);
  }
  if (bmp_filename != "") {
    save_bmp(decoder.frame.y, decoder.frame.x, bmp_filename, decoder.pixels[1], decoder.pixels[2], decoder.pixels[3]);
  }
}
