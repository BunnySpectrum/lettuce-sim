#pragma once

#include <stdint.h>

class EePoint {
 public:
  constexpr EePoint(uint8_t col, uint8_t row) : col_(col), row_(row) {}
  EePoint operator+(const EePoint& other) const;
  constexpr uint8_t row() const { return row_; }
  constexpr uint8_t col() const { return col_; }

 private:
  uint8_t col_;
  uint8_t row_;
};