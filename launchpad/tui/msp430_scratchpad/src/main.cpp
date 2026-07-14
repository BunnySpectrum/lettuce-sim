#include "arduino/byte_stream_arduino.h"
#include "cmd_processor.h"
#include "ee_composer.h"
#include "ee_frame.h"
#include "ee_message.h"
#include "ee_point.h"
#include "ee_text.h"
#include "locale.h"

#include <Arduino.h>
#undef OUTPUT
#undef INPUT
#include <limits.h>
#include <msp430.h>

/* Stack */
#define STACK_PAINT_COLOR 0xAC1D
// see .platformio/packages/toolchain-timsp430/msp430/lib/ldscripts/msp430.xbn
// Ram goes data, bss, noinit
extern unsigned char __data_start[];  // start of RAM
extern unsigned char __noinit_end[];
const unsigned int stack_end = (unsigned int)__noinit_end;
const unsigned int kRamUsed = stack_end - (unsigned int)__data_start;

void paint_stack() {
  volatile unsigned int sp;
  __asm__("MOV R1, %0" : "=r"(sp));
  unsigned int idx;
  for (idx = sp; idx > stack_end; idx -= 2) {
    *((unsigned int*)idx) = STACK_PAINT_COLOR;
  }
}
#define kVectorSize 32
#define kRomOrigin 0xC000
extern unsigned char __ctors_start[];
extern unsigned char _etext[];
extern unsigned char __data_size[];
const unsigned int kFlashUsed =
    ((unsigned int)_etext - kRomOrigin) + (unsigned int)__data_size + kVectorSize;

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

struct StackRoom room_setup, room_hbt, room_debug, room_compose;
/* end */

ByteStreamEnergia console_uart(0);
EeComposer _composer(&console_uart);
#define VT05

/* Locale */
uint8_t language = LANG_EN;
uint8_t prev_language = LANG_EN;

int8_t locale_string_delta(const char* const* locale_strings, uint8_t prev, uint8_t current) {
  if (prev_language != language) {
    uint8_t prev_width = TextWidth(locale_strings[prev]);
    uint8_t new_width = TextWidth(locale_strings[current]);
    return prev_width - new_width;
  } else {
    return 0;
  }
}
/* end */

uint16_t last_millis;
uint32_t uptime_ms = 0;
uint16_t compose_duration_ms = 0;
uint8_t debug_state = 0;
uint8_t input_count = 0;
bool needs_redraw = false;
void print_debug(const EeComposer& composer);
uint8_t debug_task_counter;
#define DEBUG_TASK_PERIOD 200

/* View */
#ifdef VT05
#define kScreenWidth 72
#define kScreenHeight 20
#elif defined(VT52)
#define kScreenWidth 80
#define kScreenHeight 24
#elif defined(VT100)
#define kScreenWidth 80
#define kScreenHeight 24
#endif

struct AppView {
  uint8_t width;
  uint8_t height;
};

const uint8_t kTopMargin = 0;
const uint8_t kLeftMargin = 1;
const uint8_t kMiddleMargin = 1;
const uint8_t kMiddleGap = 2;
const EePoint kHomePoint = EePoint(1 /*col*/, 1 /*row*/);
const EePoint kDebugPoint = EePoint(kScreenWidth + 10, 1 /* row */);
const EePoint kInputPoint = EePoint(kLeftMargin + 2, kDebugPoint.row());

const struct AppView kAppMemView = {16 * 4 + 4, 1 + 16};
const struct AppView kAppControlsView = {3, 1 + 13};
const struct AppView kAppDecodeView = {32, 3};

const EePoint kNwPoint = EePoint(1, 3);
const EePoint kNePoint = EePoint(kAppControlsView.width + 2 /*col*/, 1 /*row*/);
const EePoint kSwPoint = EePoint(1, kAppMemView.height + 1);

/* end */

/* Input */
char last_input = '\0';
CmdProcessorState cmd_state;
const char* active_cmd;
uint8_t cmd_idx;
EeText input_text = EeText(kInputPoint);
int ReadCommand(uint8_t* character) {
  const char byte = console_uart.ReadByte();
  if (byte < 0x20) {
    return -1;
  }
  *character = byte;
  return 0;
}

/* end */

/* Kenbak */
#define MEM_SIZE 256
uint8_t kenbakMemory[MEM_SIZE];
enum class KenbakReg : uint8_t {
  A = 000,
  B = 001,
  X = 002,
  PC = 003,
  OUTPUT = 0200,
  AOC = 0201,
  BOC = 0202,
  XOC = 0203,
  INPUT = 0377,
};
bool redraw_memory = true;
char* const kHbtValues[] = {"/", "\\"};
uint8_t hbt_index = 0;

// How many ticks (today, 1tick = 1ms) until we run
//  >1 = decrement each tick
//  1 = ready to run
//  0 = do not update counter each tick
#define HBT_TASK_PERIOD 200
uint8_t hbt_task_counter;
EeText hbt_text = EeText(kDebugPoint);
/* end */

void app_decode(const EeComposer& composer) {
  composer.ComposeStringC("Decode");
  composer.MoveLeft(6);
  composer.MoveDown(1);
  if (last_input > 0x20) {
    composer.stream_->WriteChar(last_input);
  }
}
void app_controls(const EeComposer& composer) {
#define RN_CONTROL                             \
  do {                                         \
    composer.MoveLeft(kAppControlsView.width); \
    composer.MoveDown(1);                      \
  } while (0);

  composer.ComposeStringC("PWR");
  RN_CONTROL

  composer.stream_->WriteStringC("_01");
  RN_CONTROL

  composer.stream_->WriteStringC("INP");
  RN_CONTROL

  composer.stream_->WriteStringC("_C+");
  RN_CONTROL

  composer.stream_->WriteStringC("ADD");
  RN_CONTROL

  composer.stream_->WriteStringC("_S+");
  RN_CONTROL

  composer.stream_->WriteStringC("_R+");
  RN_CONTROL

  composer.stream_->WriteStringC("MEM");
  RN_CONTROL

  composer.stream_->WriteStringC("_S+");
  RN_CONTROL

  composer.stream_->WriteStringC("_R+");
  RN_CONTROL

  composer.stream_->WriteStringC("RUN");
  RN_CONTROL

  composer.stream_->WriteStringC("_Y+");
  RN_CONTROL

  composer.stream_->WriteStringC("_N+");
  RN_CONTROL

  composer.stream_->WriteStringC("_S_");
  RN_CONTROL

#undef RN_CONTROL
}

void app_mem_draw_all(const EeComposer& composer) {
  // Example to print w/ locale change
  // composer.ComposeStringC(kTimeStrings[language]);
  // composer.stream_->Write(0x20);
  // composer.stream_->WriteDWord(uptime_ms);

  // int8_t delta = locale_string_delta(kTimeStrings, prev_language, language);
  // if (delta > 0) {
  //   composer.ClearChars(delta);
  // }
  // composer.ClearToEndOfLine();

  {  // Write column headings
    composer.stream_->WriteStringC("    ");
    for (uint8_t col = 0; col < 16; col++) {
      composer.stream_->WriteChar(' ');
      composer.stream_->WriteOct(col);
    }
    // composer.stream_->WriteChar('|');
    composer.MoveLeft(kAppMemView.width);
    composer.MoveDown(1);
  }
  for (uint8_t row = 0; row < 16; row++) {

    // write address
    // composer.stream_->WriteHex(row * 0x10);
    composer.stream_->WriteOct(row * 0x10);
    composer.stream_->WriteChar(':');

    // Write the 0xF values
    for (uint8_t col = 0; col < 16; col++) {
      uint8_t value = kenbakMemory[row * 0x10 + col];
      composer.stream_->WriteChar(' ');
      composer.stream_->WriteOct(value);
    }
    // composer.stream_->WriteChar('|');

    {  // Moveto next row
      // baseline
      // composer.stream_->WriteStringC("\x1b[");
      // composer.stream_->WriteByte(row + 2 + 1 + (kNwPoint.row() - 1));
      // composer.stream_->WriteChar(';');
      // composer.stream_->WriteByte(kNwPoint.col());
      // composer.stream_->WriteChar('H');

      // +8 stack
      // composer.MoveTo(row + 2 + (kNwPoint.row() - 1), 1);

      // +6 stack
      // composer.MoveTo(kNwPoint);
      // composer.MoveDown(row + 2);

      // +4 stack
      composer.MoveLeft(kAppMemView.width);
      composer.MoveDown(1);
    }
  }
  redraw_memory = false;
}

void ComposeSheet(const EeComposer& composer) {
  uint16_t start = millis();

  digitalWrite(RED_LED, true);

  // Decision: ee_frame helps keep consistent views, but is less flexible, and uses more ram+stack
  {
    // nw_frame.Compose(composer);  // baseline (390 RAM, 54 stack)

    // -6 RAM, -2 stack
    // composer.MoveTo(kNwPoint);
    // composer.ComposeDiv(kAppMemWidth);
    // composer.MoveTo(kNwPoint);
    // composer.MoveDown(1);
    // composer.MoveRight(1);
    // app_mem();
    // composer.MoveTo(kNwPoint);
    // composer.MoveDown(kAppMemHeight);
    // composer.ComposeDiv(kAppMemWidth);
  }

  {  // NW app
    composer.MoveTo(kNwPoint);
    app_controls(composer);
  }

  if (redraw_memory) {  // NE app
    composer.MoveTo(kNePoint);
    app_mem_draw_all(composer);
  }

  {  // SE app
    composer.MoveTo(kSwPoint);
    app_decode(composer);
  }
  digitalWrite(RED_LED, false);
  compose_duration_ms = millis() - start;
}

void setup() {
  paint_stack();
  room_setup.update_pre();
  console_uart.Configure(115200);  // -38 to room

  hbt_task_counter = HBT_TASK_PERIOD;
  debug_task_counter = DEBUG_TASK_PERIOD;

  pinMode(RED_LED, 1);
  pinMode(GREEN_LED, 1);

  input_text.SetText("");

  hbt_text.SetText(kHbtValues[hbt_index]);
  // composer.ClearScreen();
  _composer.stream_->WriteStringC("\x1b[2J");
  _composer.ShowCursor(false);

  last_millis = millis();
  room_setup.update_post();
  needs_redraw = true;
}

void task_hbt(const EeComposer& composer) {
  {  // 42
    // composer.MoveDown(pc / 16 + 1);
  }

  {  // 38
    // composer.stream_->WriteStringC("\x1b[");
    // composer.stream_->WriteByte(pc / 16 + 1);
    // composer.stream_->WriteChar('B');
  }

  {  // 38
    // composer.stream_->WriteChar('\x1b');
    // composer.stream_->WriteChar('[');
    // composer.stream_->WriteByte(pc / 16 + 1);
    // composer.stream_->WriteChar('B');
  }

  {  // 20
    // composer.stream_->WriteChar('\x1b');
    // composer.stream_->WriteChar('[');
    // composer.stream_->WriteChar('1');
    // composer.stream_->WriteChar('B');
  }

  {  // 20
    // composer.stream_->WriteChar('B');
  }

  const uint8_t pc = static_cast<uint8_t>(KenbakReg::PC);
  kenbakMemory[pc]++;
  composer.MoveTo(kNePoint);
  composer.MoveDown(pc / 16 + 1);
  composer.MoveRight(4 + 4 * pc + 1);
  composer.stream_->WriteOct(kenbakMemory[pc]);

  // const uint8_t output = 0377;
  // kenbakMemory[output] += 2;
  // composer.MoveTo(kNePoint);
  // composer.MoveDown(output / 16 + 1);
  // composer.MoveRight(4 + 4 * (0337 & 0xf) + 1);
  // composer.stream_->WriteOct(kenbakMemory[output]);

  // needs_redraw = true;
}

uint16_t ms_since_last_check(uint16_t* last_millis) {
  const uint16_t new_millis = millis();
  uint16_t result;

  if (new_millis >= *last_millis) {
    result = new_millis - *last_millis;
  } else {
    result = *last_millis + new_millis + (UINT_MAX - *last_millis);
  }

  *last_millis = new_millis;
  return result;
}

void loop() {
  // setup -> loop, no extra stack

  delay(1);

  uptime_ms += ms_since_last_check(&last_millis);  //not it
  if (console_uart.Available()) {
    last_input = console_uart.ReadByte();
    needs_redraw = true;
    if (last_input == 'r') {
      redraw_memory = true;
    }
  }

  // removing did +4 to room
  // if (console_uart.Available()) {
  //   // console_uart.ReadByte();
  //   // digitalWrite(GREEN_LED, debug_state ^= 0x1);

  //   // }
  //   buf[0] = console_uart.ReadByte();
  //   buf[1] = '\0';
  //   input_text.SetTextRaw(buf);
  //   input_count = 1;
  //   needs_redraw = true;
  //   switch (buf[0]) {
  //     case 'E':
  //       language = LANG_EN;
  //       break;
  //     case 'R':
  //       language = LANG_RU;
  //       break;
  //   }
  // }

  // if (false) {
  //   uint8_t cmd = buf[0];

  //   if (-1 != cmd) {

  //     if (cmd != ' ') {
  //       //input_text.Relocate(kInputOrigin + EePoint(input_count, 0 /*row*/));
  //       input_count++;
  //       input_text.SetText((char*)&cmd);
  //     }
  //     switch (cmd_state) {
  //       case kReady:
  //         if (cmd == kCmdHelp[cmd_idx]) {
  //           cmd_state = kReceiving;
  //           active_cmd = kCmdHelp;
  //         } else if (cmd == kCmdLangEn[cmd_idx]) {
  //           cmd_state = kReceiving;
  //           active_cmd = kCmdLangEn;
  //         } else if (cmd == kCmdLangRu[cmd_idx]) {
  //           cmd_state = kReceiving;
  //           active_cmd = kCmdLangRu;
  //         } else {
  //           active_cmd = 0;
  //         }
  //         break;
  //       case kReceiving:
  //         if (cmd == ' ') {
  //           break;
  //         }
  //         cmd_idx++;
  //         if ((0 == active_cmd[cmd_idx]) || (cmd != active_cmd[cmd_idx])) {
  //           // gone past the command or mismatched
  //           cmd_state = kDone;
  //           active_cmd = 0;
  //         }
  //         break;
  //       case kDone:
  //         break;
  //     }
  //   } else {

  //     // Saw a frame start after receiving text
  //     //input_text.Relocate(kInputOrigin);
  //     composer.MoveTo(kInputOrigin);
  //     composer.ClearChars(input_count);
  //     input_text.SetText("");

  //     bool valid_cmd = false;
  //     if (active_cmd == kCmdLangEn) {
  //       language = LANG_EN;
  //       valid_cmd = true;
  //     } else if (active_cmd == kCmdLangRu) {
  //       language = LANG_RU;
  //       valid_cmd = true;
  //     }
  //     if (valid_cmd) {
  //       status_text.SetText(kAckStrings[language]);
  //     } else {
  //       status_text.SetText(kErrorStrings[language]);
  //     }

  //     cmd_state = kReady;
  //     input_count = 0;
  //     cmd_idx = 0;
  //   }

  //   // Draw
  //   needs_redraw = true;

  //   prev_language = language;
  // }

  if (hbt_task_counter-- == 1) {
    room_hbt.update_pre();
    task_hbt(_composer);
    room_hbt.update_post();
    hbt_task_counter = HBT_TASK_PERIOD;
  }

  if (needs_redraw) {
    _composer.ShowCursor(false);  // if host clears the terminal, we need to re-send

    room_compose.update_pre();
    ComposeSheet(_composer);
    room_compose.update_post();

    needs_redraw = false;
  }

  if (debug_task_counter-- == 1) {
    room_debug.update_pre();
    print_debug(_composer);
    room_debug.update_post();
    debug_task_counter = DEBUG_TASK_PERIOD;
  }
}
void print_debug(const EeComposer& composer) {
  hbt_index++;
  hbt_index %= sizeof(kHbtValues) / sizeof(char*);
  hbt_text.SetText(kHbtValues[hbt_index]);
  hbt_text.Compose(composer);  // -34 to room

  uint8_t line = 0;

  // Uptime
  composer.MoveTo(kDebugPoint);
  composer.MoveDown(++line);
  composer.ComposeStringC("Uptime (ms): ");
  composer.stream_->WriteDWord(uptime_ms);

  // Last compose duration
  composer.MoveTo(kDebugPoint);
  composer.MoveDown(++line);
  composer.ComposeStringC("Compose (ms): ");
  composer.stream_->WriteWord(compose_duration_ms);
  composer.ClearToEndOfLine();

  // Amount of memory used
  composer.MoveTo(kDebugPoint);
  composer.MoveDown(++line);
  composer.ComposeStringC("Flash / Ram: ");
  composer.stream_->WriteWord(kFlashUsed);
  composer.ComposeStringC(" / ");
  composer.stream_->WriteWord(kRamUsed);

  // Remaining stack we could use
  composer.MoveTo(kDebugPoint);
  composer.MoveDown(++line);
  composer.ComposeStringC("Room pre/post/delta: ");

  composer.MoveTo(kDebugPoint);
  composer.MoveDown(++line);
  composer.ComposeStringC(" Setup: ");
  room_setup.print(composer);

  composer.MoveTo(kDebugPoint);
  composer.MoveDown(++line);
  composer.ComposeStringC(" HBT: ");
  room_hbt.print(composer);

  composer.MoveTo(kDebugPoint);
  composer.MoveDown(++line);
  composer.ComposeStringC(" Compose: ");
  room_compose.print(composer);

  composer.MoveTo(kDebugPoint);
  composer.MoveDown(++line);
  composer.ComposeStringC(" Debug: ");
  room_debug.print(composer);
}
