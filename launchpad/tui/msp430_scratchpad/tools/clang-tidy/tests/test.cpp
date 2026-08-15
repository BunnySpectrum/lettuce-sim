#include "bad_header.h"
#include "good_header.h"

int makeRuntimeValue() { return 7; }

int main() { return HeaderPoint.Column + RuntimeInteger + LiteralInteger; }

