#include <stdint.h>

class Point {
 public:
  constexpr Point(uint8_t col, uint8_t row) : col_(col), row_(row) {}
  constexpr uint8_t row() const { return row_; }
  constexpr uint8_t col() const { return col_; }

 private:
  uint8_t col_;
  uint8_t row_;
};

constexpr Point kPoint(1, 3);

uint8_t read_row() { return kPoint.row(); }

