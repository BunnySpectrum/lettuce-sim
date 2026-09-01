#include "ee_point.h"

EePoint EePoint::operator+(const EePoint& other) const {
  return EePoint{
      static_cast<uint8_t>(col + other.col),
      static_cast<uint8_t>(row + other.row),
  };
}
