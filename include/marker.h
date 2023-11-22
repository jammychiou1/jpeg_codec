#ifndef MARKER_H
#define MARKER_H

typedef std::array<uint8_t, 2> marker_t;

const marker_t SOI = marker_t{0xff, 0xd8};
const marker_t EOI = marker_t{0xff, 0xd9};
const marker_t COM = marker_t{0xff, 0xfe};
const marker_t SOF0 = marker_t{0xff, 0xc0};
const marker_t DRI = marker_t{0xff, 0xdd};
const marker_t DHT = marker_t{0xff, 0xc4};
const marker_t DQT = marker_t{0xff, 0xdb};
const marker_t SOS = marker_t{0xff, 0xda};

const std::array<marker_t, 16> APP = [] {
  std::array<marker_t, 16> result{};
  for (int i = 0; i < 16; i++) {
    result[i] = {0xff, (uint8_t)(0xe0 + i)};
  }
  return result;
} ();

const std::array<marker_t, 8> RST = [] {
  std::array<marker_t, 8> result{};
  for (int i = 0; i < 8; i++) {
    result[i] = {0xff, (uint8_t)(0xd0 + i)};
  }
  return result;
} ();

#endif // MARKER_H
