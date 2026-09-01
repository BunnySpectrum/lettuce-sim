#pragma once

#include <stdint.h>

struct EePoint {
  EePoint operator+(const EePoint& other) const;

  uint8_t col;
  uint8_t row;
};
