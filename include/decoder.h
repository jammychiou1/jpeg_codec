#ifndef DECODER_H
#define DECODER_H

#include "huffman.h"
#include "quantize.h"

struct component_param_t {
  int h;
  int v;
  int q;
};

struct frame_param_t {
  int p;
  int y;
  int x;
  int n_comp;
  int hmax;
  int vmax;
};

struct scan_comp_param_t {
  int id;
  int td;
  int ta;
};

struct scan_param_t {
  int n_scan_comp;
  scan_comp_param_t scan_comps[4];
};

struct decoder_state_t {
  frame_param_t frame;
  scan_param_t scan;
  component_param_t comps[4];
  quant_table qtabs[4];
  huffman_lut* htabs[2][4];
};

#endif // DECODER_H
