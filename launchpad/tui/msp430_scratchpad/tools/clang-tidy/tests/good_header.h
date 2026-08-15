#ifndef LETTUCE_TIDY_TEST_GOOD_HEADER_H
#define LETTUCE_TIDY_TEST_GOOD_HEADER_H

struct SharedPoint;

extern const SharedPoint DeclaredOnlyPoint;
extern const int DeclaredOnlyInteger;
inline const int InlineInteger = 42;

#endif

