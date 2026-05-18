#include "byte_stream_energia.h"
#include "cmd_processor.h"
#include "ee_composer.h"
#include "ee_frame.h"
#include "ee_message.h"
#include "ee_point.h"
#include "ee_text.h"
#include "locale.h"

#include <Arduino.h>
#include <limits.h>
#include <msp430.h>

// #define P2S_SH_LDL P1_4
// #define P2S_CLK P2_3
// #define P2S_OUT P2_4

// #define S2P_SER P2_5
// #define S2P_SRCLK P2_6
// #define S2P_RCLK P2_7

#define VT05

#ifdef VT05
#define kScreenWidth 72
#define kScreenHeight 20
#elif defined(VT50)
#define kScreenWidth 80
#define kScreenHeight 12
#elif defined(VT52)
#define kScreenWidth 80
#define kScreenHeight 24
#elif defined(VT100)
#define kScreenWidth 80
#define kScreenHeight 24
#endif

#define STACK_PAINT_COLOR 0xAC1D

ByteStreamEnergia console_uart(0);
EeComposer composer(&console_uart);
uint8_t language = LANG_EN;
uint8_t prev_language = LANG_EN;
uint8_t input_count = 0;
uint32_t uptime_ms = 0;
uint16_t compose_duration_ms = 0;
uint8_t debug_state = 0;
bool needs_redraw = false;
char buf[2];

// void PulseS2PSrclk(uint8_t count) {
//   digitalWrite(S2P_SRCLK, false);
//   for (uint8_t i = 0; i < count; i++) {
//     digitalWrite(S2P_SRCLK, true);
//     delayMicroseconds(1);
//     digitalWrite(S2P_SRCLK, false);
//     delayMicroseconds(1);
//   }
// }

// void GetReport() {
// Serial.write(ESC);
// Serial.write(CSI);
// Serial.write('6');
// Serial.write('n');
// console_uart.WriteStringC("\x1b[6n");
// String line = Serial.readStringUntil('R');
// uint8_t idx = 2;
// for (idx = 2; idx < line.length(); idx++) {
//   Serial.print(String(line.charAt(idx)));
// }
// Serial.print("\r\n");
// char row = line[2];
// char col = line[4];
// report = String("Row: " + String(row) + ", Col: " + String(col) + ".");
// Serial.println(report);
// }

// see .platformio/packages/toolchain-timsp430/msp430/lib/ldscripts/msp430.xbn
// Ram goes data, bss, noinit
extern unsigned char __data_start[];  // start of RAM
extern unsigned char __noinit_end[];
const unsigned int stack_end = (unsigned int)__noinit_end;
const unsigned int kRamUsed = stack_end - (unsigned int)__data_start;

#define kVectorSize 32
#define kRomOrigin 0xC000
extern unsigned char __ctors_start[];
extern unsigned char _etext[];
extern unsigned char __data_size[];
const unsigned int kFlashUsed =
    ((unsigned int)_etext - kRomOrigin) + (unsigned int)__data_size + kVectorSize;

const uint8_t kTopMargin = 0;
const uint8_t kLeftMargin = 1;
const uint8_t kMiddleMargin = 1;
const uint8_t kMiddleGap = 2;
const EePoint kHomePoint = EePoint(1 /*col*/, 1 /*row*/);
const EePoint kDebugPoint = EePoint(1 /*col*/, kScreenHeight + 2);
const EePoint kInputPoint = EePoint(kLeftMargin + 2, kDebugPoint.row());

const uint8_t kAppMemWidth = 16 * 3 + 3 + 2;
const uint8_t kAppMemHeight = 16 + 1;

const uint8_t kAppSideWidth = kScreenWidth - kAppMemWidth;
const uint8_t kAppSideHeight = kAppMemHeight;

// Using char* const was -2B RAM, +30B Flash
char* const kHbtValues[] = {"/", "\\"};
uint8_t hbt_index = 0;
#define MEM_SIZE 256
uint8_t kenbakMemory[MEM_SIZE];
bool redraw_memory = true;

uint16_t ram_room = 0;
bool sample_room = true;
uint16_t pre_room, post_room, room_setup_pre, room_setup_post;
// FormatData ram_room_data = FormatData(uint16_t(0));
// LogMessage ram_room_log = {kRoomStrings, &ram_room_data};

int8_t locale_string_delta(const char* const* locale_strings, uint8_t prev, uint8_t current) {
  if (prev_language != language) {
    uint8_t prev_width = TextWidth(locale_strings[prev]);
    uint8_t new_width = TextWidth(locale_strings[current]);
    return prev_width - new_width;
  } else {
    return 0;
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

void app_mem() {
  // Example to print w/ locale change
  // composer.ComposeStringC(kTimeStrings[language]);
  // composer.stream_->Write(0x20);
  // composer.stream_->WriteDWord(uptime_ms);

  // int8_t delta = locale_string_delta(kTimeStrings, prev_language, language);
  // if (delta > 0) {
  //   composer.ClearChars(delta);
  // }
  // composer.ClearToEndOfLine();

  if (redraw_memory) {
    for (uint8_t row = 0; row < 16; row++) {

      // write address
      composer.stream_->WriteHex(row * 0x10);
      composer.stream_->Write(':');

      // Write the 0xF values
      for (uint8_t col = 0; col < 16; col++) {
        uint8_t value = kenbakMemory[row * 0x10 + col];
        composer.stream_->Write(' ');
        composer.stream_->WriteHex(value);
      }

      // composer.MoveTo(row + 1 + 2, 2);
      composer.stream_->WriteStringC("\x1b[");
      composer.stream_->WriteByte(row + 1 + 2);
      composer.stream_->Write(';');
      composer.stream_->WriteByte(2);
      composer.stream_->Write('H');
    }

    redraw_memory = true;
  }
}

//const EePoint kInputOrigin = EePoint(kHomePoint.col() + kLeftMargin, kHomePoint.row() + kTopMargin);

EeText hbt_text = EeText(kDebugPoint);

EeText input_text = EeText(kInputPoint);
//EeFrame ne_frame = EeFrame(kHomePoint, kAppMemWidth, kAppMemHeight, app_mem);

void ComposeSheet() {
  uint16_t start = millis();

  digitalWrite(RED_LED, true);

  // ne_frame.Compose(composer);
  composer.MoveTo(kHomePoint);
  composer.ComposeBar(true /*is_top*/, kAppMemWidth);
  composer.MoveTo(kHomePoint);
  composer.MoveRight(1);
  composer.MoveDown(1);
  app_mem();
  composer.MoveTo(kHomePoint);
  composer.MoveDown(kAppMemHeight);
  composer.ComposeBar(false /*is_top*/, kAppMemWidth);

  digitalWrite(RED_LED, false);
  compose_duration_ms = millis() - start;
}

CmdProcessorState cmd_state;

int ReadCommand(uint8_t* character) {
  const char byte = console_uart.ReadByte();
  if (byte < 0x20) {
    return -1;
  }
  *character = byte;
  return 0;
}
uint16_t room_max = 0;
void print_debug() {
  hbt_text.Compose(composer);

  // composer.MoveTo(kDebugPoint);
  composer.stream_->WriteStringC("\x1b[");
  composer.stream_->WriteByte(kDebugPoint.row());
  composer.stream_->Write(';');
  composer.stream_->WriteByte(kDebugPoint.col());
  composer.stream_->Write('H');

  // Last entered character
  // composer.MoveRight(2);
  composer.stream_->WriteStringC("\x1b[");
  composer.stream_->WriteByte(2);
  composer.stream_->Write('C');
  composer.ComposeStringC(buf);

  // Uptime
  composer.ComposeStringC("\r\nUptime (ms): ");
  composer.stream_->WriteDWord(uptime_ms);

  // Last compose duration
  composer.ComposeStringC("\r\nCompose (ms): ");
  composer.stream_->WriteWord(compose_duration_ms);
  composer.ClearToEndOfLine();

  // Amount of RAM used
  composer.ComposeStringC("\r\nRam used: ");
  composer.stream_->WriteWord(kRamUsed);

  // Amount of flash used
  composer.ComposeStringC("\r\nFlash used: ");
  composer.stream_->WriteWord(kFlashUsed);

  // Remaining stack we could use
  composer.ComposeStringC("\r\nRoom: ");
  // composer.stream_->WriteWord(ram_room_log.data->data.word);
  // #define WORD ram_room_log.data->data.word
  //   uint8_t post_clear = 0;
  //   // Only 512B ram, so only need to check up to 3 digits
  //   if (WORD < 100) {
  //     post_clear++;
  //     if (WORD < 10) {
  //       post_clear++;
  //     }
  //   }
  //   composer.ClearChars(post_clear);
  composer.stream_->WriteWord(ram_room);
  composer.ClearToEndOfLine();

  composer.ComposeStringC("\r\nRoom (setup_pre): ");
  composer.stream_->WriteWord(room_setup_pre);

  composer.ComposeStringC("\r\nRoom (setup_post): ");
  composer.stream_->WriteWord(room_setup_post);

  composer.ComposeStringC("\r\nRoom (debug_pre): ");
  composer.stream_->WriteWord(pre_room);

  composer.ComposeStringC("\r\nRoom (debug_post): ");
  composer.stream_->WriteWord(post_room);

  composer.ComposeStringC("\r\nRoom (setup delta): ");
  composer.stream_->WriteWord(room_setup_pre - room_setup_post);

  composer.ComposeStringC("\r\nRoom (debug delta): ");
  composer.stream_->WriteWord(pre_room - post_room);

  composer.ComposeStringC("\r\nPC: ");
  composer.stream_->WriteWord(kenbakMemory[3]);
}
void print_room() {

  composer.stream_->WriteWord(room_max);
  composer.stream_->Write('>');
  // composer.stream_->WriteWord(ram_room_log.data->data.word);
  composer.stream_->WriteWord(ram_room);

  composer.stream_->Write('.');
}

// How many ticks (today, 1tick = 1ms) until we run
//  >0 = decrement each tick
//  0 = ready to run
//  <0 = do not update counter each tick
#define HBT_TASK_PERIOD 20
#define HBT_UPDATE_PERIOD 50
uint8_t hbt_task_counter;
uint8_t hbt_update_counter;

uint16_t last_millis;
const char* active_cmd;
uint8_t cmd_idx;

void setup() {
  volatile unsigned int sp;
  __asm__("MOV R1, %0" : "=r"(sp));
  unsigned int idx;
  for (idx = sp; idx > stack_end; idx -= 2) {
    *((unsigned int*)idx) = STACK_PAINT_COLOR;
  }
  room_setup_pre = task_update_room();  //130
  console_uart.Configure(115200);       // -38 to room
  // console_uart.stream_->begin(115200);   // -36
  room_setup_post = task_update_room();  // 92

  hbt_task_counter = HBT_TASK_PERIOD;
  hbt_update_counter = 0;

  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  input_text.SetText("");

  hbt_text.SetText(kHbtValues[hbt_index]);
  // composer.ClearScreen();
  composer.stream_->WriteStringC("\x1b[2J");
  composer.ShowCursor(false);

  last_millis = millis();
  pre_room = task_update_room();
}

// void test_recursion(uint8_t level) {
//   __asm__("MOV R1, %0" : "=r"(sp));
//   composer.MoveTo(EePoint(1, 29));
//   composer.ComposeStringC("L: ");
//   composer.stream_->WriteWord(sp);
//   composer.ComposeStringC(": ");
//   composer.stream_->WriteByte(level);
//   composer.ComposeStringC(".");
//   if (level == 0) {
//     return;
//   }
//   test_recursion(--level);
// }

void task_hbt() {
  hbt_index++;
  hbt_index %= sizeof(kHbtValues) / sizeof(char*);
  hbt_text.SetText(kHbtValues[hbt_index]);

  needs_redraw = true;
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

  delay(1);  // not it

  // removing did +4 to room
  uptime_ms += ms_since_last_check(&last_millis);  //not it

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

  if (hbt_task_counter > 0) {
    hbt_task_counter--;
  } else if (hbt_task_counter == 0) {

    ram_room = task_update_room();  // not it

    if (ram_room > room_max) {
      room_max = ram_room;
    }

    if (hbt_update_counter++ > HBT_UPDATE_PERIOD) {
      hbt_update_counter = 0;
      //task_hbt();
    }
    if (sample_room) {
      pre_room = task_update_room();
    }
    print_debug();
    if (sample_room) {
      post_room = task_update_room();
      sample_room = false;
    }
    // print_room();

    hbt_task_counter = HBT_TASK_PERIOD;
  }
  if (needs_redraw) {
    //composer.ShowCursor(false);  // if host clears the terminal, we need to re-send
    // ComposeSheet();

    needs_redraw = false;
  }
}
