#pragma once
#include "ee_point.h"

class EeNode {
 public:
  EeNode(EePoint origin) : origin_(origin) {};
  void Relocate(const EePoint& point) { origin_ = point; }

  EePoint origin() const { return origin_; }

 private:
  EePoint origin_;
};