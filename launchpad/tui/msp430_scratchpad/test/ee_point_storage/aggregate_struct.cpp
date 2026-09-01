#include <stdint.h>

struct Point {
  uint8_t col;
  uint8_t row;
};

constexpr Point kPoint = {1, 3};

uint8_t read_row() { return kPoint.row; }

