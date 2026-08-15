#ifndef LETTUCE_TIDY_TEST_BAD_HEADER_H
#define LETTUCE_TIDY_TEST_BAD_HEADER_H

int makeRuntimeValue();

struct Point {
  Point(int Column, int Row) : Column(Column), Row(Row) {}
  int Column;
  int Row;
};

const Point HeaderPoint(1, 2);
const int RuntimeInteger = makeRuntimeValue();
const int LiteralInteger = 42;

#endif

