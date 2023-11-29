#include <string>
#include "parser.h"

// int decode_coef(int len, int bits) {
//   if (len == 0) {
//     return 0;
//   }
//   if (bits & (1 << (len - 1))) {
//     return bits;
//   }
//   int low = bits & low_k_mask(len - 1);
//   return 1 - (1 << len) + low;
// }
//
// const std::array<std::pair<int, int>, 64> zigzag = [] {
//   int i = 0, j = 0;
//   int dir = -1;
//   std::array<std::pair<int, int>, 64> res = {};
//   for (int t = 1; t < 64; t++) {
//     if (i + dir < 0 || i + dir >= 8 || j - dir < 0 || j - dir >= 8) {
//       if (dir == -1) {
//         if (j + 1 < 8) {
//           j++;
//         }
//         else {
//           i++;
//         }
//       }
//       else {
//         if (i + 1 < 8) {
//           i++;
//         }
//         else {
//           j++;
//         }
//       }
//       dir *= -1;
//     }
//     else {
//       i += dir;
//       j -= dir;
//     }
//     res[t] = {i, j};
//   }
//   return res;
// } ();
//
// void decode_block(unstuffing_bitstream& bs, const huffman_lut& lut_dc, const huffman_lut& lut_ac, int16_t coefs[8][8]) {
//   for (int i = 0; i < 8; i++) {
//     for (int j = 0; j < 8; j++) {
//       coefs[i][j] = 0;
//     }
//   }
//
//   // std::cerr << "DC:\n";
//   {
//     int cate = lut_dc.lookup(bs);
//     int diff = decode_coef(cate, bs.get_k(cate));
//     // std::cerr << "  " << diff << '\n';
//     coefs[0][0] = diff;
//   }
//
//   // std::cerr << "AC:\n";
//   {
//     // std::cerr << " lut @ " << (void*)&lut << '\n';
//     for (int t = 1; t < 64; t++) {
//       int tmp = lut_ac.lookup(bs);
//       if (tmp == 0x00) {
//         // std::cerr << "(EOB)\n";
//         break;
//       }
//       int run = tmp / 16;
//       for (int dt = 0; dt < run; dt++) {
//         coefs[zigzag[t].first][zigzag[t].second] = 0;
//         t++;
//       }
//       int cate = tmp % 16;
//       int diff = decode_coef(cate, bs.get_k(cate));
//       coefs[zigzag[t].first][zigzag[t].second] = diff;
//
//       // std::cerr << "  " << t << "  " << diff << '\n';
//     }
//   }
// }
//
// void decode_scan_segment(mapped_file mmap, int left, int right) {
//   int num_mcu = 0;
//   int du_per_mcu = 0;
//   if (scan.n_components == 1) {
//     int cols = (frame.width - 1) / 8 + 1;
//     int rows = (frame.height - 1) / 8 + 1;
//     num_mcu = cols * rows;
//     du_per_mcu = 1;
//   }
//   else {
//     int cols = (frame.width - 1) / (frame.hmax * 8) + 1;
//     int rows = (frame.height - 1) / (frame.vmax * 8) + 1;
//     num_mcu = cols * rows;
//     for (int j = 0; j < scan.n_components; j++) {
//       component_info_t component = frame.components[scan.ids[j]];
//       du_per_mcu += component.h * component.v;
//     }
//   }
//   // std::cerr << "expecting " << num_mcu << " MCUs, " << du_per_mcu << " DUs per MCU\n";
//
//   unstuffing_bitstream bs;
//   bs.set_mmap(mmap, left, right);
//
//   std::array<int, 4> last_dcs = {};
//   int now = left;
//   int16_t coefs[8][8];
//   uint8_t pxls[8][8];
//   for (int t = 0; t < num_mcu; t++) {
//     // std::cerr << "MCU " << t << '\n';
//     if (scan.n_components == 1) {
//       decode_block(bs, *htabs[0][scan.tds[0]], *htabs[1][scan.tas[0]], coefs);
//     }
//     else {
//       for (int j = 0; j < scan.n_components; j++) {
//         // std::cerr << "component id: " << scan.ids[j] << '\n';
//
//         component_info_t component = frame.components[scan.ids[j]];
//         const quant_table& qtab = qtabs[component.q];
//         for (int y = 0; y < component.v; y++) {
//           for (int x = 0; x < component.h; x++) {
//             // std::cerr << "DU " << x << ' ' << y << '\n';
//
//             decode_block(bs, *htabs[0][scan.tds[j]], *htabs[1][scan.tas[j]], coefs);
//             coefs[0][0] += last_dcs[j];
//             last_dcs[j] = coefs[0][0];
//
//             for (int t = 0; t < 64; t++) {
//               coefs[zigzag[t].first][zigzag[t].second] *= qtab.table[t];
//             }
//
//             // std::cout << "coefs:\n";
//             // for (int u = 0; u < 8; u++) {
//             //   for (int v = 0; v < 8; v++) {
//             //     std::cout << coefs[u][v] << ' ';
//             //   }
//             //   std::cout << '\n';
//             // }
//
//             idct(coefs, pxls);
//
//             // std::cout << "pixels:\n";
//             if (j == 0) {
//               for (int u = 0; u < 8; u++) {
//                 for (int v = 0; v < 8; v++) {
//                   std::cout << int(pxls[u][v]) << ' ';
//                 }
//                 std::cout << '\n';
//               }
//             }
//           }
//         }
//       }
//     }
//   }
// }

// bool parse_segment_done = false;
// int parse_segment(mapped_file mmap, int offset) {
//   marker_t marker{mmap[offset], mmap[offset + 1]};
//   if (marker == SOI) {
//     // std::cerr << "SOI\n";
//     return 2;
//   }
//   if (marker == EOI) {
//     // std::cerr << "EOI\n";
//     parse_segment_done = true;
//     return 2;
//   }
//   if (marker == COM) {
//     // std::cerr << "COM\n";
//     int size = read_segment_size(mmap, offset + 2);
//     return size + 2;
//   }
//   if (marker == SOF0) {
//     // std::cerr << "SOF0\n";
//     return read_frame(mmap, offset);
//   }
//   for (int i = 0; i < 16; i++) {
//     if (marker == APP[i]) {
//       // std::cerr << "APP" << i << '\n';
//       int size = read_segment_size(mmap, offset + 2);
//       return size + 2;
//     }
//   }
//   if (marker == DRI) {
//     // std::cerr << "DRI\n";
//     int size = read_segment_size(mmap, offset + 2);
//     return size + 2;
//   }
//   if (marker == DHT) {
//     // std::cerr << "DHT\n";
//     return read_huffman_table(mmap, offset);
//   }
//   if (marker == DQT) {
//     // std::cerr << "DQT\n";
//     return read_quantization_table(mmap, offset);
//   }
//   if (marker == SOS) {
//     // std::cerr << "SOS\n";
//     return read_scan(mmap, offset);
//   }
//   print_byte(std::cerr, marker[0]);
//   std::cerr << ' ';
//   print_byte(std::cerr, marker[1]);
//   std::cerr << '\n';
//   throw std::logic_error("unknown marker");
// }

int main(int argc, char** argv) {
  if (argc < 2) {
    return 1;
  }
  std::string filename = argv[1];
  load_file(filename);
}
