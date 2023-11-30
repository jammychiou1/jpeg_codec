#ifndef DECODER_H
#define DECODER_H

#include "huffman.h"
#include "quantize.h"

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
};

struct scan_info_t {
  int n_components;
  int ids[4];
  int tds[4];
  int tas[4];
};

struct decoder_state_t {
  frame_info_t frame_info;
  scan_info_t scan_info;
  quant_table qtabs[4];
  huffman_lut* htabs[2][4];
  component_info_t components[4];
};

extern decoder_state_t decoder_state;

#endif // DECODER_H
