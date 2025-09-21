#pragma once

#include <stdint.h>

class EePoint {
 public:
  EePoint(uint8_t col, uint8_t row);
  EePoint(const EePoint& other);
  EePoint operator+(const EePoint& other) const;
  uint8_t row() const { return row_; }
  uint8_t col() const { return col_; }

 private:
  uint8_t col_;
  uint8_t row_;
};