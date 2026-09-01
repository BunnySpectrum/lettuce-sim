#include <stdint.h>

struct Point {
  constexpr Point(uint8_t col_value, uint8_t row_value)
      : col(col_value), row(row_value) {}

  uint8_t col;
  uint8_t row;
};

constexpr Point kPoint(1, 3);

uint8_t read_row() { return kPoint.row; }

