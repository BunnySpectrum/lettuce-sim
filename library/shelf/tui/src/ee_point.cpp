#include "ee_point.h"

EePoint EePoint::operator+(const EePoint& other) const {
  return EePoint(this->col_ + other.col_, this->row_ + other.row_);
}