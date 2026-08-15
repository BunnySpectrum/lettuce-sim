#include "stack.h"

const unsigned int stack_end = (unsigned int)__noinit_end;
const unsigned int kRamUsed = stack_end - (unsigned int)__data_start;
const unsigned int kFlashUsed =
    ((unsigned int)_etext - kRomOrigin) + (unsigned int)__data_size + kVectorSize;

struct StackRoom room_setup, room_hbt, room_debug, room_compose;

void paint_stack() {
  volatile unsigned int sp;
  __asm__("MOV R1, %0" : "=r"(sp));
  unsigned int idx;
  for (idx = sp; idx > stack_end; idx -= 2) {
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
  for (idx = sp; idx > stack_end; idx -= 2) {
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