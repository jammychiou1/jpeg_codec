#ifndef DECODER_H
#define DECODER_H

#include <array>
#include <cstdint>
#include <vector>

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
};

struct scan_comp_param_t {
  int id;
  int td;
  int ta;
};

struct du_layout_t {
  int y_mcu;
  int x_mcu;
  int y_scan_comp_du_per_mcu[4];
  int x_scan_comp_du_per_mcu[4];
  int n_du_per_mcu;
};

struct scan_state_t {
  int last_dcs[4];
  int i_mcu;
  int j_mcu;
  int now_mcu;
  int k_scan_comp;
  int i_du;
  int j_du;

  std::vector<std::array<int16_t, 64>> coefs[4];
};

struct scan_param_t {
  int n_scan_comp;
  scan_comp_param_t scan_comps[4];
  du_layout_t du_layout;

  scan_state_t scan_state;
};

struct decoder_state_t {
  frame_param_t frame;
  component_param_t comps[4];
  quant_table* qtabs[4];
  huffman_lut* htabs[2][4];

  scan_param_t scan;

  std::vector<uint8_t> pixels[4];
};

void process_scan(decoder_state_t& dcd);

#endif // DECODER_H
