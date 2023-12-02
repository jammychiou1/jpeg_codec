#ifndef PARSER_H
#define PARSER_H

#include <cstdint>
#include <string>

#include "decoder.h"

enum marker_type {SOI, EOI, COM, APPn, RSTn, SOFn, DRI, DHT, DQT, SOS};

struct marker_segment_t {
  marker_type mrk;
  int sub;
  int off;
  int len;
};

struct parser_state_t {
  const uint8_t* ptr;
  int size;
  marker_segment_t mrk_seg;
  decoder_state_t* dcd;
};

void parse_image(parser_state_t& psr);

#endif // PARSER_H
