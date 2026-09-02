#ifndef STACK_H
#define STACK_H
#include <stdint.h>
#include "ee_composer.h"

#define STACK_PAINT_COLOR 0xAC1D

// see .platformio/packages/toolchain-timsp430/msp430/lib/ldscripts/msp430.x
// Ram goes data, bss, noinit
extern unsigned char __data_start[];  // start of RAM
extern unsigned char __noinit_end[];
uint16_t ram_used();

#define kVectorSize 32
#define kRomOrigin 0xC000
extern unsigned char __ctors_start[];
extern unsigned char _etext[];
extern unsigned char __data_size[];
uint16_t flash_used();

void paint_stack();
int task_update_room();

struct StackRoom {
  uint16_t pre;
  uint16_t post;

  void update_pre() {
    paint_stack();
    pre = task_update_room();
  }

  void update_post() { post = task_update_room(); }

  void print(const EeComposer& composer) const {
    composer.stream_->WriteWord(pre);
    composer.ComposeStringC(" / ");
    composer.stream_->WriteWord(post);
    composer.ComposeStringC(" / ");
    composer.stream_->WriteWord(pre - post);
    composer.ClearToEndOfLine();
  }
};

extern struct StackRoom room_setup, room_hbt, room_debug, room_compose;

#endif
