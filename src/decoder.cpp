#include "decoder.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>
#include <vector>
using std::vector, std::array, std::pair;
using std::max, std::min, std::round;

#include "dct_impl.h"
#define DCT_IMPL DCT_IMPL_SSSE3
#include "dct.h"

#include "bmp_writer.h"

#include <iostream>
using std::cerr, std::cout;

const array<pair<int, int>, 64> zigzag = [] {
  int i = 0, j = 0;
  int dir = -1;
  array<pair<int, int>, 64> res = {};
  for (int t = 1; t < 64; t++) {
    if (i + dir < 0 || i + dir >= 8 || j - dir < 0 || j - dir >= 8) {
      if (dir == -1) {
        if (j + 1 < 8) j++;
        else i++;
      }
      else {
        if (i + 1 < 8) i++;
        else j++;
      }
      dir *= -1;
    }
    else {
      i += dir;
      j -= dir;
    }
    res[t] = {i, j};
  }
  return res;
} ();

void process_scan(decoder_state_t& dcd) {
  frame_param_t& frame = dcd.frame;
  scan_param_t& scan = dcd.scan;
  component_param_t* comps = &dcd.comps[0];

  int vmax = 0;
  int hmax = 0;
  for (int k = 0; k < frame.n_comp; k++) {
    component_param_t& comp = comps[k];
    vmax = max(vmax, comp.v);
    hmax = max(hmax, comp.h);
  }

  // cerr << "now processing...\n";
  du_layout_t& du_layout = scan.du_layout;
  for (int k = 0; k < scan.n_scan_comp; k++) {
    int& comp_id = scan.scan_comps[k].id;
    component_param_t& comp = comps[comp_id];
    int v_upsamp = vmax / comp.v;
    int h_upsamp = hmax / comp.h;

    vector<vector<uint8_t>>& pixels = dcd.pixels[comp_id];
    pixels = vector<vector<uint8_t>>(frame.y, vector<uint8_t>(frame.x));

    // cerr << "scan comp " << k << ", comp id = " << comp_id << '\n';
    // cerr << "  v upsamp = " << v_upsamp << ", h upsamp = " << h_upsamp << '\n';
    // cerr << "  du rows = " << du_layout.y_scan_comp_du[k] << ", cols = " << du_layout.x_scan_comp_du[k] << '\n';

    int now_scan_comp_du = 0;
    vector<array<int16_t, 64>>& coefs = scan.scan_state.coefs[k];
    for (int i_du = 0; i_du < du_layout.y_scan_comp_du[k]; i_du++) {
      for (int j_du = 0; j_du < du_layout.x_scan_comp_du[k]; j_du++) {
        array<int16_t, 64>& coef_list = coefs[now_scan_comp_du];
        // for (int t = 0; t < 64; t++) {
        //   cerr << coef_list[t] << ' ';
        // }
        // cerr << '\n';

        int16_t coef_block[8][8] = {};
        for (int t = 0; t < 64; t++) {
          coef_block[zigzag[t].first][zigzag[t].second] = coef_list[t];
        }
        uint8_t pixel_block[8][8] = {};
        idct(coef_block, pixel_block);

        for (int i_pxl = 0; i_pxl < 8; i_pxl++) {
          for (int j_pxl = 0; j_pxl < 8; j_pxl++) {
            for (int i_sub = 0; i_sub < v_upsamp; i_sub++) {
              for (int j_sub = 0; j_sub < h_upsamp; j_sub++) {
                int y = (i_du * 8 + i_pxl) * v_upsamp + i_sub;
                int x = (j_du * 8 + j_pxl) * h_upsamp + j_sub;
                if (0 <= y && y < frame.y && 0 <= x && x < frame.x) {
                  pixels[y][x] = pixel_block[i_pxl][j_pxl];
                }
              }
            }
          }
        }

        now_scan_comp_du++;
      }
      // cerr << '\n';
    }
  }
}

void process_image(decoder_state_t& dcd) {
  frame_param_t& frame = dcd.frame;
  component_param_t* comps = &dcd.comps[0];

  bmp_writer wrt;
  wrt.new_file("test.bmp", frame.y, frame.x);

  vector<vector<uint8_t>>* pixels = &dcd.pixels[0];
  for (int i = 0; i < frame.y; i++) {
    for (int j = 0; j < frame.x; j++) {
      double y = pixels[1][i][j];
      double cb = pixels[2][i][j];
      double cr = pixels[3][i][j];

      int r = round(y + 1.402 * (cr - 128));
      int g = round(y - 0.344136 * (cb - 128) - 0.714136 * (cr - 128));
      int b = round(y + 1.402 * (cb - 128));
      r = max(0, min(255, r));
      g = max(0, min(255, g));
      b = max(0, min(255, b));

      int begin = 54 + (frame.y - 1 - i) * wrt.padded_width + j * 3;
      wrt.ptr[begin + 2] = r;
      wrt.ptr[begin + 1] = g;
      wrt.ptr[begin + 0] = b;
    }
  }
}
