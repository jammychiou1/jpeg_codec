#include "parser.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <utility>

#include "decoder.h"
#include "ecs_bitstream.h"
#include "huffman.h"
#include "quantize.h"
#include "util.h"

using std::max;
using std::array, std::vector, std::pair;

#include <iostream>
using std::cerr;

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

void read_u4_2(const uint8_t* ptr, int& off, int& high, int& low) {
  int b = ptr[off];
  high = (b >> 4) & 0xf;
  low = b & 0xf;
  off++;
}

void read_u8(const uint8_t* ptr, int& off, int& val) {
  val = ptr[off];
  off++;
}

void read_u16(const uint8_t* ptr, int& off, int& val) {
  int high = ptr[off];
  int low = ptr[off + 1];
  val = (high << 8) | low;
  off += 2;
}

void setup_huffman_table(parser_state_t& psr) {
  decoder_state_t* dcd = psr.dcd;
  const uint8_t* ptr = psr.ptr;
  int off = psr.mrk_seg.off + 2;
  int len = psr.mrk_seg.len;
  int end = off + len;
  off += 2;

  while (off < end) {
    int Tc, Th;
    read_u4_2(ptr, off, Tc, Th);
    if (Tc < 0 || Tc > 1) throw std::logic_error("bad Tc");
    if (Th < 0 || Th > 3) throw std::logic_error("bad Th");
    cerr << "Tc, Th: " << Tc << ", " << Th << '\n';

    int Li_s[16];
    for (int i = 0; i < 16; i++) {
      read_u8(ptr, off, Li_s[i]);
      cerr << "L" << i << ": " << Li_s[i] << '\n';
    }

    // TODO separate lut compilation function
    // TODO properly free
    huffman_lut* htab = new huffman_lut();
    int code = 0;
    for (int i = 0; i < 16; i++) {
      for (int j = 0; j < Li_s[i]; j++) {
        int Vij;
        read_u8(ptr, off, Vij);

        htab->add_codeword(code, i + 1, Vij);
        code++;
      }
      code *= 2;
    }

    dcd->htabs[Tc][Th] = htab;
  }
  if (off != end) throw std::logic_error("bad table size");
}

void setup_quantization_table(parser_state_t& psr) {
  decoder_state_t* dcd = psr.dcd;
  const uint8_t* ptr = psr.ptr;
  int off = psr.mrk_seg.off + 2;
  int len = psr.mrk_seg.len;
  int end = off + len;
  off += 2;

  while (off < end) {
    int Pq, Tq;
    read_u4_2(ptr, off, Pq, Tq);
    if (Pq < 0 || Pq > 1) throw std::logic_error("bad Pq");
    if (Tq < 0 || Tq > 3) throw std::logic_error("bad Tq");
    cerr << "Pq, Tq: " << Pq << ", " << Tq << '\n';

    int Qk_s[64];
    for (int k = 0; k < 64; k++) {
      if (Pq == 0) read_u8(ptr, off, Qk_s[k]);
      else read_u16(ptr, off, Qk_s[k]);
      cerr << "Q" << k << ": " << Qk_s[k] << '\n';
    }

    quant_table* qtab = new quant_table();
    *qtab = {};
    qtab->p = Pq;
    for (int k = 0; k < 64; k++) {
      qtab->arr[k] = Qk_s[k];
    }
    dcd->qtabs[Tq] = qtab;
  }

  if (off != end) throw std::logic_error("bad table size");
}

void setup_du_layout(parser_state_t& psr) {
  du_layout_t du_layout = {};

  decoder_state_t* dcd = psr.dcd;
  frame_param_t& frame = dcd->frame;
  scan_param_t& scan = dcd->scan;

  int y = frame.y;
  int x = frame.x;
  if (scan.n_scan_comp == 1) {
    du_layout.y_mcu = (y - 1) / 8 + 1;
    du_layout.x_mcu = (x - 1) / 8 + 1;

    du_layout.y_scan_comp_du_per_mcu[0] = 1;
    du_layout.x_scan_comp_du_per_mcu[0] = 1;
    du_layout.n_du_per_mcu = 1;
  }
  else {
    int vmax = 0;
    int hmax = 0;
    for (int i = 0; i < frame.n_comp; i++) {
      component_param_t& comp = dcd->comps[i];
      vmax = max(vmax, comp.v);
      hmax = max(hmax, comp.h);
    }

    du_layout.y_mcu = (y - 1) / (vmax * 8) + 1;
    du_layout.x_mcu = (x - 1) / (hmax * 8) + 1;

    for (int j = 0; j < scan.n_scan_comp; j++) {
      int comp_id = scan.scan_comps[j].id;
      component_param_t& comp = dcd->comps[comp_id];

      du_layout.y_scan_comp_du_per_mcu[j] = comp.v;
      du_layout.x_scan_comp_du_per_mcu[j] = comp.h;
      du_layout.n_du_per_mcu += comp.v * comp.h;
    }
  }
  // cerr << "expecting " << num_mcu << " MCUs, " << du_per_mcu << " DUs per MCU\n";

  scan.du_layout = du_layout;
};


void setup_scan_param(parser_state_t& psr) {
  decoder_state_t* dcd = psr.dcd;
  const uint8_t* ptr = psr.ptr;
  int off = psr.mrk_seg.off + 2;
  int len = psr.mrk_seg.len;
  int end = off + len;
  off += 2;

  scan_param_t scan = {};

  int Ns;
  read_u8(ptr, off, Ns);
  if (Ns > 4) throw std::logic_error("bad Ns");
  cerr << "Ns: " << Ns << '\n';

  scan.n_scan_comp = Ns;

  for (int j = 0; j < Ns; j++) {
    int Csj, Tdj, Taj;
    read_u8(ptr, off, Csj);
    read_u4_2(ptr, off, Tdj, Taj);
    if (Tdj < 0 || Tdj > 3) throw std::logic_error("bad Tdj");
    if (Taj < 0 || Taj > 3) throw std::logic_error("bad Taj");
    cerr << "Cs" << j << ": " << Csj << '\n';
    cerr << "Td" << j << ", Ta" << j << ": " << Tdj << ", " << Taj << '\n';

    scan_comp_param_t scan_comp;
    scan_comp.id = Csj;
    scan_comp.td = Tdj;
    scan_comp.ta = Taj;

    scan.scan_comps[j] = scan_comp;
  }

  int Ss, Se, Ah, Al;
  read_u8(ptr, off, Ss);
  read_u8(ptr, off, Se);
  read_u4_2(ptr, off, Ah, Al);
  cerr << "Ss: " << Ss << '\n';
  cerr << "Se: " << Se << '\n';
  cerr << "Ah, Al: " << Ah << ", " << Al << '\n';

  if (off != end) throw std::logic_error("bad scan header size");

  dcd->scan = scan;
  setup_du_layout(psr);
}

void setup_frame_param(parser_state_t& psr) {
  decoder_state_t* dcd = psr.dcd;
  const uint8_t* ptr = psr.ptr;
  int off = psr.mrk_seg.off + 2;
  int len = psr.mrk_seg.len;
  int end = off + len;
  off += 2;

  frame_param_t frame = {};

  int P, Y, X, Nf;
  read_u8(ptr, off, P);
  read_u16(ptr, off, Y);
  read_u16(ptr, off, X);
  read_u8(ptr, off, Nf);
  if (Nf <= 0 || Nf > 4) throw std::logic_error("bad component count");
  cerr << "P: " << P << '\n';
  cerr << "Y: " << Y << '\n';
  cerr << "X: " << X << '\n';
  cerr << "Nf: " << Nf << '\n';

  frame.p = P;
  frame.y = Y;
  frame.x = X;
  frame.n_comp = Nf;

  for (int i = 0; i < Nf; i++) {

    int Ci, Hi, Vi, Tqi;
    read_u8(ptr, off, Ci);
    read_u4_2(ptr, off, Hi, Vi);
    read_u8(ptr, off, Tqi);
    if (Ci > 4) throw std::runtime_error("large id not supported");
    if (Hi != 1 && Hi != 2 && Hi != 4) throw std::logic_error("bad Hi");
    if (Vi != 1 && Vi != 2 && Vi != 4) throw std::logic_error("bad Vi");
    cerr << "C" << i << ": " << Ci << '\n';
    cerr << "H" << i << ", V" << i << ": " << Hi << ", " << Vi << '\n';
    cerr << "Tq" << i << ": " << Tqi << '\n';

    component_param_t comp = {};
    comp.h = Hi;
    comp.v = Vi;
    comp.q = Tqi;
    dcd->comps[Ci] = comp;
  }

  if (off != end) throw std::logic_error("bad frame header size");

  dcd->frame = frame;
}

int decode_coef(int len, int bits) {
  if (len == 0) return 0;
  if (bits & (1 << (len - 1))) return bits;
  int low = bits & low_k_mask(len - 1);
  return 1 - (1 << len) + low;
}

void put_coefs_du(parser_state_t& psr, int16_t coefs_du[64]) {
  scan_param_t& scan = psr.dcd->scan;

  scan_state_t& scan_state = scan.scan_state;
  int& i_mcu = scan_state.i_mcu;
  int& j_mcu = scan_state.j_mcu;
  int& i_du = scan_state.i_du;
  int& j_du = scan_state.j_du;
  int& k_scan_comp = scan_state.k_scan_comp;
  int& comp_id = scan.scan_comps[k_scan_comp].id;
  vector<array<int16_t, 64>>& coefs = scan_state.coefs[k_scan_comp];

  du_layout_t& du_layout = scan.du_layout;
  int& y_mcu = du_layout.y_mcu;
  int& x_mcu = du_layout.x_mcu;
  int& y_scan_comp_du_per_mcu = du_layout.y_scan_comp_du_per_mcu[comp_id];
  int& x_scan_comp_du_per_mcu = du_layout.x_scan_comp_du_per_mcu[comp_id];

  int y_scan_comp_du = y_mcu * y_scan_comp_du_per_mcu;
  int x_scan_comp_du = x_mcu * x_scan_comp_du_per_mcu;
  int i_scan_comp_du = i_mcu * y_scan_comp_du_per_mcu + i_du;
  int j_scan_comp_du = j_mcu * x_scan_comp_du_per_mcu + j_du;
  int loc = i_scan_comp_du * x_scan_comp_du + j_scan_comp_du;
  for (int t = 0; t < 64; t++) {
    coefs[loc][t] = coefs_du[t];
  }
}

void decode_du(parser_state_t& psr, ecs_bitstream& bs, const huffman_lut* htab_dc, const huffman_lut* htab_ac, const quant_table* qtab) {
  int16_t coefs_du[64] = {};

  cerr << "DC:\n";
  {
    int cate = bs.get_huffman(htab_dc);
    int diff = decode_coef(cate, bs.get_k(cate));
    coefs_du[0] = diff;
    cerr << "  " << diff << '\n';

    scan_param_t& scan = psr.dcd->scan;
    scan_state_t& scan_state = scan.scan_state;
    int& k_scan_comp = scan_state.k_scan_comp;
    int& last_dc = scan_state.last_dcs[k_scan_comp];
    coefs_du[0] += last_dc;
    last_dc = coefs_du[0];
  }

  cerr << "AC:\n";
  {
    for (int t = 1; t < 64; t++) {
      int tmp = bs.get_huffman(htab_ac);
      if (tmp == 0x00) break; // EOB

      int run = tmp / 16;
      t += run;

      int cate = tmp % 16;
      int diff = decode_coef(cate, bs.get_k(cate));
      coefs_du[t] = diff;
      cerr << "  " << t << "  " << diff << '\n';
    }
  }

  for (int t = 0; t < 64; t++) {
    coefs_du[t] *= qtab->arr[t];
  }
}

void decode_mcu(parser_state_t& psr, ecs_bitstream& bs) {
  scan_param_t& scan = psr.dcd->scan;
  du_layout_t& du_layout = scan.du_layout;

  scan_state_t& scan_state = scan.scan_state;
  int& k = scan_state.k_scan_comp;
  int& i = scan_state.i_du;
  int& j = scan_state.j_du;

  for (k = 0; k < scan.n_scan_comp; k++) {
    cerr << "scan comp " << k << '\n';
    scan_comp_param_t& scan_comp = scan.scan_comps[k];
    component_param_t& comp = psr.dcd->comps[scan_comp.id];
    const huffman_lut* htab_dc = psr.dcd->htabs[0][scan_comp.td];
    const huffman_lut* htab_ac = psr.dcd->htabs[1][scan_comp.ta];
    const quant_table* qtab = psr.dcd->qtabs[comp.q];

    int y_scan_comp_du_per_mcu = du_layout.y_scan_comp_du_per_mcu[k];
    int x_scan_comp_du_per_mcu = du_layout.x_scan_comp_du_per_mcu[k];
    for (i = 0; i < y_scan_comp_du_per_mcu; i++) {
      for (j = 0; j < x_scan_comp_du_per_mcu; j++) {
        decode_du(psr, bs, htab_dc, htab_ac, qtab);
      }
    }
  }
}

void decode_ecs(parser_state_t& psr, int begin, int end) {
  ecs_bitstream bs(psr.ptr, begin, end);

  scan_param_t& scan = psr.dcd->scan;
  du_layout_t& du_layout = scan.du_layout;

  scan_state_t& scan_state = scan.scan_state;
  int& i = scan_state.i_mcu;
  int& j = scan_state.j_mcu;
  int& t = scan_state.now_mcu;

  int y_mcu = du_layout.y_mcu;
  int x_mcu = du_layout.x_mcu;
  for (t = 0; t < y_mcu * x_mcu; t++) {
    cerr << "MCU " << t << '\n';
    decode_mcu(psr, bs);
    j++;
    if (j >= x_mcu) {
      j = 0;
      i++;
    }
  }
}

bool parse_marker_segment(parser_state_t& psr, int& off) {
  const uint8_t* ptr = psr.ptr;
  marker_segment_t mrk_seg = {};
  mrk_seg.off = off;

  int tmp;
  read_u16(ptr, off, tmp);
  if (tmp >> 8 != 0xff) return false;

  int b = tmp & 0xff;
  switch (b) {
  case 0xd8:
    mrk_seg.mrk = SOI; break;
  case 0xd9:
    mrk_seg.mrk = EOI; break;
  case 0xfe:
    mrk_seg.mrk = COM; break;
  case 0xdd:
    mrk_seg.mrk = DRI; break;
  case 0xc4:
    mrk_seg.mrk = DHT; break;
  case 0xdb:
    mrk_seg.mrk = DQT; break;
  case 0xda:
    mrk_seg.mrk = SOS; break;
  default:
    if ((b & 0xf0) == 0xc0) {
      mrk_seg.mrk = SOFn;
      mrk_seg.sub = b & 0x0f;
      break;
    }
    if ((b & 0xf0) == 0xe0) {
      mrk_seg.mrk = APPn;
      mrk_seg.sub = b & 0x0f;
      break;
    }
    if ((b & 0xf1) == 0xd0) {
      mrk_seg.mrk = RSTn;
      mrk_seg.sub = b & 0x07;
      break;
    }
    return false;
  }

  switch (mrk_seg.mrk) {
  case COM:
  case DRI:
  case DHT:
  case DQT:
  case SOS:
  case SOFn:
  case APPn:
    int len;
    read_u16(ptr, off, len);
    if (len < 2) return false;
    mrk_seg.len = len;
    off += len - 2;
  }
  psr.mrk_seg = mrk_seg;
  return true;
}

bool parse_misc(parser_state_t& psr, int& off) {
  int new_off = off;
  if (!parse_marker_segment(psr, new_off)) return false;
  switch (psr.mrk_seg.mrk) {
  case APPn:
  case DRI:
  case COM:
    off = new_off;
    return true;
  }
  return false;
}

bool parse_scan_tbls(parser_state_t& psr, int& off) {
  int new_off = off;
  if (!parse_marker_segment(psr, new_off)) return false;
  switch (psr.mrk_seg.mrk) {
  case DHT:
    setup_huffman_table(psr);
    off = new_off;
    return true;
  case DQT:
    setup_quantization_table(psr);
    off = new_off;
    return true;
  }
  return false;
}

void parse_scan_prelude(parser_state_t& psr, int& off) {
  bool flag;
  do {
    flag = false;
    flag |= parse_misc(psr, off);
    flag |= parse_scan_tbls(psr, off);
  } while (flag);
}

void parse_scan_hdr(parser_state_t& psr, int& off) {
  parse_marker_segment(psr, off);
  if (psr.mrk_seg.mrk != SOS) throw std::logic_error("expecting scan header");
  setup_scan_param(psr);
}

void init_scan_state(parser_state_t& psr) {
  scan_param_t& scan = psr.dcd->scan;

  scan_state_t scan_state = {};

  du_layout_t& du_layout = scan.du_layout;
  int y_mcu = du_layout.y_mcu;
  int x_mcu = du_layout.x_mcu;
  for (int k = 0; k < scan.n_scan_comp; k++) {
    int y_scan_comp_du_per_mcu = du_layout.y_scan_comp_du_per_mcu[k];
    int x_scan_comp_du_per_mcu = du_layout.x_scan_comp_du_per_mcu[k];
    scan_state.coefs[k].resize(y_mcu * x_mcu * y_scan_comp_du_per_mcu * x_scan_comp_du_per_mcu);
  }

  scan.scan_state = scan_state;
}

void parse_scan_body(parser_state_t& psr, int& off) {
  init_scan_state(psr);

  const uint8_t* ptr = psr.ptr;
  int l = off, r;
  bool done = false;
  do {
    r = l;
    while (true) {
      if (ptr[r] != 0xff) {
        r++;
        continue;
      }
      if (ptr[r + 1] == 0x00) {
        r += 2;
        continue;
      }
      break;
    }

    decode_ecs(psr, l, r);

    done = true;
    if ((ptr[r + 1] & 0xf1) == 0xd0) {
      done = false;
      l = r + 2;
    }
  } while (!done);
  off = r;
}

void parse_scan(parser_state_t& psr, int& off) {
  parse_scan_prelude(psr, off);
  parse_scan_hdr(psr, off);
  parse_scan_body(psr, off);

  process_scan(*psr.dcd);
}

int parse_frame_tbls(parser_state_t& psr, int& off) {
  int new_off = off;
  if (!parse_marker_segment(psr, new_off)) return false;
  switch (psr.mrk_seg.mrk) {
  case DHT:
    setup_huffman_table(psr);
    off = new_off;
    return true;
  case DQT:
    setup_quantization_table(psr);
    off = new_off;
    return true;
  }
  return false;
}

void parse_frame_prelude(parser_state_t& psr, int& off) {
  bool flag;
  do {
    flag = false;
    flag |= parse_misc(psr, off);
    flag |= parse_frame_tbls(psr, off);
  } while (flag);
}

void parse_frame_hdr(parser_state_t& psr, int& off) {
  parse_marker_segment(psr, off);
  if (psr.mrk_seg.mrk != SOFn) throw std::logic_error("expecting frame header");
  setup_frame_param(psr);
}

void parse_frame(parser_state_t& psr, int& off) {
  parse_frame_prelude(psr, off);
  parse_frame_hdr(psr, off);
  parse_scan(psr, off);
}

void parse_image(parser_state_t& psr) {
  int off = 0;

  parse_marker_segment(psr, off);
  if (psr.mrk_seg.mrk != SOI) throw std::logic_error("expecting SOI");

  parse_frame(psr, off);

  parse_marker_segment(psr, off);
  if (psr.mrk_seg.mrk != EOI) throw std::logic_error("expecting EOI");
}
