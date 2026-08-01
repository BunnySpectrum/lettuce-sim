#include "arduino/byte_stream_arduino.h"
#include "cmd_processor.h"
#include "cpu.h"
#include "ee_composer.h"
#include "ee_message.h"
#include "ee_point.h"
#include "ee_text.h"
#include "locale.h"
#include "stack.h"
#include "view.h"
#include "app.h"

#include <Arduino.h>
#undef OUTPUT
#undef INPUT
#include <limits.h>
#include <msp430.h>

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
bool debug_enabled = true;
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

// App app(_composer);
App app = {};
const uint8_t kLeftMargin = 1;
// const EePoint kHomePoint = EePoint(1 /*col*/, 1 /*row*/);
const EePoint kDebugPoint = EePoint(kScreenWidth + 10, 1 /* row */);
const EePoint kInputPoint = EePoint(kLeftMargin + 2, kDebugPoint.row());



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
const char* const kHbtValues[] = {"/", "\\"};
uint8_t hbt_index = 0;
Kenbak cpuState;

// How many ticks (today, 1tick = 1ms) until we run
//  >1 = decrement each tick
//  1 = ready to run
//  0 = do not update counter each tick
#define CPU_TASK_PERIOD 200
uint8_t cpu_task_counter;
EeText hbt_text = EeText(kDebugPoint);
/* end */



void ComposeSheet(const EeComposer& composer) {
  uint16_t start = millis();

  digitalWrite(RED_LED, true);

  // app.app_controls();
  // app.app_mem_draw_all(cpuState);
  // app.app_decode(cpuState);
  app.app_controls(_composer);
  app.app_mem_draw_all(cpuState, _composer);
  app.app_decode(cpuState, _composer);

  digitalWrite(RED_LED, false);
  compose_duration_ms = millis() - start;
}

void setup() {
  paint_stack();
  room_setup.update_pre();
  console_uart.Configure(115200);  // -38 to room

  cpu_task_counter = CPU_TASK_PERIOD;
  debug_task_counter = DEBUG_TASK_PERIOD;

  pinMode(RED_LED, 1);
  pinMode(GREEN_LED, 1);

  input_text.SetText("");

  hbt_text.SetText(kHbtValues[hbt_index]);
  _composer.ClearScreen();
  _composer.ShowCursor(false);

  last_millis = millis();
  room_setup.update_post();
  app.mem_view_cursor_set(cpuState.cursor_address(), _composer);
}

void task_cpu(const EeComposer& composer) {
  if (cpuState.step()) {
    cpuState.execute();
    // app.mem_view_update_addr(cpuState, static_cast<uint8_t>(KenbakReg::PC));
    app.mem_view_update_addr(cpuState, static_cast<uint8_t>(KenbakReg::PC), _composer);
    app.request_update_decode();
  }
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

  uptime_ms += ms_since_last_check(&last_millis); 
  if (console_uart.Available()) {
    last_input = console_uart.ReadByte();
    switch (last_input) {
      case 'r':
        app.mem_view_cursor_set(cpuState.cursor_address(), _composer);
        app.request_update_all();
        break;
      case 'g':
        cpuState.toggle_step();
        break;
      case 's':
        // case 'j':
        app.mem_view_cursor_clear(cpuState.cursor_address(), _composer);
        cpuState.move_cursor_address(16);
        app.mem_view_cursor_set(cpuState.cursor_address(), _composer);
        app.request_update_decode();
        break;
      case 'w':
        // case 'k':
        app.mem_view_cursor_clear(cpuState.cursor_address(), _composer);
        cpuState.move_cursor_address(-16);
        app.mem_view_cursor_set(cpuState.cursor_address(), _composer);
        app.request_update_decode();
        break;
      case 'a':
        // case 'h':
        app.mem_view_cursor_clear(cpuState.cursor_address(), _composer);
        cpuState.move_cursor_address(-1);
        app.mem_view_cursor_set(cpuState.cursor_address(), _composer);
        app.request_update_decode();
        break;
      case 'd':
        // case 'l':
        app.mem_view_cursor_clear(cpuState.cursor_address(), _composer);
        cpuState.move_cursor_address(1);
        app.mem_view_cursor_set(cpuState.cursor_address(), _composer);
        app.request_update_decode();
        break;
      case '1':
        debug_enabled ^= true;
        break;
      default:
        break;
    }
  }

  if (cpu_task_counter-- == 1) {
    room_hbt.update_pre();
    task_cpu(_composer);
    room_hbt.update_post();
    cpu_task_counter = CPU_TASK_PERIOD;
  }

  if (app.needs_redraw()) {
    _composer.ShowCursor(false);  // if host clears the terminal, we need to re-send

    room_compose.update_pre();
    ComposeSheet(_composer);
    room_compose.update_post();
  }

  if (debug_task_counter-- == 1) {
    room_debug.update_pre();
    print_debug(_composer);
    room_debug.update_post();
    debug_task_counter = DEBUG_TASK_PERIOD;
  }
}
void print_debug(const EeComposer& composer) {
  if (!debug_enabled) {
    return;
  }
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
