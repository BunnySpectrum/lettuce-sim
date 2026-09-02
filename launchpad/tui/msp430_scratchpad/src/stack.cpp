#include "stack.h"

const unsigned int stack_end = (unsigned int)__noinit_end;

uint16_t ram_used() {
  return (unsigned int)__noinit_end - (unsigned int)__data_start;
}

uint16_t flash_used() {
  return ((unsigned int)_etext - kRomOrigin) +
         (unsigned int)__data_size + kVectorSize;
}

struct StackRoom room_setup, room_hbt, room_debug, room_compose;
namespace {
  constexpr size_t kStackWordSize = sizeof(unsigned int);
}

void paint_stack() {
  volatile unsigned int sp;
  __asm__("MOV R1, %0" : "=r"(sp));
  unsigned int idx;
  for (idx = sp - kStackWordSize; idx > stack_end; idx -= kStackWordSize) {
    *((unsigned int*)idx) = STACK_PAINT_COLOR;
  }
}
int task_update_room() {
  // Check stack
  int idx;
  bool found = false;
  int result;
  volatile unsigned int sp;
  __asm__("MOV R1, %0" : "=r"(sp));
  for (idx = sp - kStackWordSize; idx > stack_end; idx -= kStackWordSize) {
    if (*((unsigned int*)idx) == STACK_PAINT_COLOR) {
      if (!found) {
        result = idx;
        found = true;
      }
    } else if (found) {
      // we previously found a false end of the stack
      // reset the 'found' flag
      found = false;
    }
  }
  if (found) {
    return result - stack_end;
  } else {
    return 0;
  }
}
