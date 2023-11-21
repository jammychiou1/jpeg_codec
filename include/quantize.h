#ifndef QUANTIZE_H
#define QUANTIZE_H

struct quant_table {
  int precision;
  std::array<int, 64> table;
};

#endif // QUANTIZE_H
