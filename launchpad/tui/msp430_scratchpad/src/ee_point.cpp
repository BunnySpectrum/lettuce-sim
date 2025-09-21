#include "ee_point.h"

EePoint::EePoint(uint8_t col, uint8_t row) : col_(col), row_(row) {}
EePoint::EePoint(const EePoint& other) : col_(other.col()), row_(other.row()) {}

EePoint EePoint::operator+(const EePoint& other) const {
  return EePoint(this->col_ + other.col_, this->row_ + other.row_);
}