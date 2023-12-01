#include "parser.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

#include "decoder.h"
#include "huffman.h"
#include "quantize.h"

using std::max;

#include <iostream>
using std::cerr;

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

void setup_huffman_table(parser_state_t& psr, decoder_state_t& dcd) {
  const uint8_t* ptr = psr.ptr;
  int off = psr.mrk_seg.off + 2;
  int len = psr.mrk_seg.len;
  int end = off + len;
  off += 2;

  while (off < end) {
    int Tc, Th;
    read_u4_2(ptr, off, Tc, Th);
    if (Tc < 0 || Tc > 1) {
      throw std::logic_error("bad Tc");
    }
    if (Th < 0 || Th > 3) {
      throw std::logic_error("bad Th");
    }
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

    dcd.htabs[Tc][Th] = htab;
  }
  if (off != end) {
    throw std::logic_error("bad table size");
  }
}

void setup_quantization_table(parser_state_t& psr, decoder_state_t& dcd) {
  const uint8_t* ptr = psr.ptr;
  int off = psr.mrk_seg.off + 2;
  int len = psr.mrk_seg.len;
  int end = off + len;
  off += 2;

  while (off < end) {
    int Pq, Tq;
    read_u4_2(ptr, off, Pq, Tq);
    if (Pq < 0 || Pq > 1) {
      throw std::logic_error("bad Pq");
    }
    if (Tq < 0 || Tq > 3) {
      throw std::logic_error("bad Tq");
    }
    cerr << "Pq, Tq: " << Pq << ", " << Tq << '\n';

    int Qk_s[64];
    for (int k = 0; k < 64; k++) {
      if (Pq == 0) {
        read_u8(ptr, off, Qk_s[k]);
      }
      else {
        read_u16(ptr, off, Qk_s[k]);
      }
      cerr << "Q" << k << ": " << Qk_s[k] << '\n';
    }

    quant_table qtab = {};
    qtab.p = Pq;
    for (int k = 0; k < 64; k++) {
      qtab.arr[k] = Qk_s[k];
    }
    dcd.qtabs[Tq] = qtab;
  }

  if (off != end) {
    throw std::logic_error("bad table size");
  }
}

void setup_scan_param(parser_state_t& psr, decoder_state_t& dcd) {
  const uint8_t* ptr = psr.ptr;
  int off = psr.mrk_seg.off + 2;
  int len = psr.mrk_seg.len;
  int end = off + len;
  off += 2;

  scan_param_t scan = {};

  int Ns;
  read_u8(ptr, off, Ns);
  if (Ns > 4) {
    throw std::logic_error("bad Ns");
  }
  cerr << "Ns: " << Ns << '\n';

  scan.n_scan_comp = Ns;

  for (int j = 0; j < Ns; j++) {
    int Csj, Tdj, Taj;
    read_u8(ptr, off, Csj);
    read_u4_2(ptr, off, Tdj, Taj);
    if (Tdj < 0 || Tdj > 3) {
      throw std::logic_error("bad Tdj");
    }
    if (Taj < 0 || Taj > 3) {
      throw std::logic_error("bad Taj");
    }
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

  if (off != end) {
    throw std::logic_error("bad scan header size");
  }

  dcd.scan = scan;
}

void setup_frame_param(parser_state_t& psr, decoder_state_t& dcd) {
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
  if (Nf <= 0 || Nf > 4) {
    throw std::logic_error("bad component count");
  }
  cerr << "P: " << P << '\n';
  cerr << "Y: " << Y << '\n';
  cerr << "X: " << X << '\n';
  cerr << "Nf: " << Nf << '\n';

  frame.p = P;
  frame.y = Y;
  frame.x = X;
  frame.n_comp = Nf;

  int Hmax = 0;
  int Vmax = 0;
  for (int i = 0; i < Nf; i++) {

    int Ci, Hi, Vi, Tqi;
    read_u8(ptr, off, Ci);
    read_u4_2(ptr, off, Hi, Vi);
    read_u8(ptr, off, Tqi);
    if (Ci > 4) {
      throw std::runtime_error("large id not supported");
    }
    if (Hi != 1 && Hi != 2 && Hi != 4) {
      throw std::logic_error("bad Hi");
    }
    if (Vi != 1 && Vi != 2 && Vi != 4) {
      throw std::logic_error("bad Vi");
    }
    cerr << "C" << i << ": " << Ci << '\n';
    cerr << "H" << i << ", V" << i << ": " << Hi << ", " << Vi << '\n';
    cerr << "Tq" << i << ": " << Tqi << '\n';

    component_param_t comp = {};
    comp.h = Hi;
    comp.v = Vi;
    Hmax = max(Hmax, Hi);
    Vmax = max(Vmax, Vi);
    comp.q = Tqi;
    dcd.comps[Ci] = comp;
  }
  frame.hmax = Hmax;
  frame.vmax = Vmax;

  if (off != end) {
    throw std::logic_error("bad frame header size");
  }

  dcd.frame = frame;
}

bool parse_marker_segment(parser_state_t& psr, int& off) {
  const uint8_t* ptr = psr.ptr;
  marker_segment_t mrk_seg = {};
  mrk_seg.off = off;

  int tmp;
  read_u16(ptr, off, tmp);
  if (tmp >> 8 != 0xff) {
    return false;
  }

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
    if (len < 2) {
      return false;
    }
    mrk_seg.len = len;
    off += len - 2;
  }
  psr.mrk_seg = mrk_seg;
  return true;
}

bool parse_misc(parser_state_t& psr, decoder_state_t& dcd, int& off) {
  int new_off = off;
  if (!parse_marker_segment(psr, new_off)) {
    return false;
  }
  switch (psr.mrk_seg.mrk) {
  case APPn:
  case DRI:
  case COM:
    off = new_off;
    return true;
  }
  return false;
}


bool parse_scan_tbls(parser_state_t& psr, decoder_state_t& dcd, int& off) {
  int new_off = off;
  if (!parse_marker_segment(psr, new_off)) {
    return false;
  }
  switch (psr.mrk_seg.mrk) {
  case DHT:
    setup_huffman_table(psr, dcd);
    off = new_off;
    return true;
  case DQT:
    setup_quantization_table(psr, dcd);
    off = new_off;
    return true;
  }
  return false;
}

void parse_scan_prelude(parser_state_t& psr, decoder_state_t& dcd, int& off) {
  bool flag;
  do {
    flag = false;
    flag |= parse_misc(psr, dcd, off);
    flag |= parse_scan_tbls(psr, dcd, off);
  } while (flag);
}

void parse_scan_hdr(parser_state_t& psr, decoder_state_t& dcd, int& off) {
  parse_marker_segment(psr, off);
  if (psr.mrk_seg.mrk != SOS) {
    throw std::logic_error("expecting scan header");
  }
  setup_scan_param(psr, dcd);
}

void parse_scan_body(parser_state_t& psr, decoder_state_t& dcd, int& off) {
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

    // decode_scan_segment(in_mmap, offset + l, offset + r);

    done = true;
    if ((ptr[r + 1] & 0xf1) == 0xd0) {
      done = false;
      l = r + 2;
    }
  } while (!done);
  off = r;
}

void parse_scan(parser_state_t& psr, decoder_state_t& dcd, int& off) {
  parse_scan_prelude(psr, dcd, off);
  parse_scan_hdr(psr, dcd, off);
  parse_scan_body(psr, dcd, off);
}

int parse_frame_tbls(parser_state_t& psr, decoder_state_t& dcd, int& off) {
  int new_off = off;
  if (!parse_marker_segment(psr, new_off)) {
    return false;
  }
  switch (psr.mrk_seg.mrk) {
  case DHT:
    setup_huffman_table(psr, dcd);
    off = new_off;
    return true;
  case DQT:
    setup_quantization_table(psr, dcd);
    off = new_off;
    return true;
  }
  return false;
}

void parse_frame_prelude(parser_state_t& psr, decoder_state_t& dcd, int& off) {
  bool flag;
  do {
    flag = false;
    flag |= parse_misc(psr, dcd, off);
    flag |= parse_frame_tbls(psr, dcd, off);
  } while (flag);
}

void parse_frame_hdr(parser_state_t& psr, decoder_state_t& dcd, int& off) {
  parse_marker_segment(psr, off);
  if (psr.mrk_seg.mrk != SOFn) {
    throw std::logic_error("expecting frame header");
  }
  setup_frame_param(psr, dcd);
}

void parse_frame(parser_state_t& psr, decoder_state_t& dcd, int& off) {
  parse_frame_prelude(psr, dcd, off);
  parse_frame_hdr(psr, dcd, off);
  parse_scan(psr, dcd, off);
}

void parse_image(parser_state_t& psr, decoder_state_t& dcd) {
  int off = 0;

  parse_marker_segment(psr, off);
  if (psr.mrk_seg.mrk != SOI) {
    throw std::logic_error("expecting SOI");
  }

  parse_frame(psr, dcd, off);

  parse_marker_segment(psr, off);
  if (psr.mrk_seg.mrk != EOI) {
    throw std::logic_error("expecting EOI");
  }
}
