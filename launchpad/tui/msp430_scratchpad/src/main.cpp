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

#define kScreenWidth 80
#define kScreenHeight 25
#define STACK_PAINT_COLOR 0xABCD

ByteStreamEnergia console_uart(0);
EeComposer composer(&console_uart);
int language = LANG_EN;
int prev_language = LANG_EN;
uint8_t input_count = 0;
uint32_t uptime_ms;
uint8_t debug_state = 0;

void app_nop(const EeComposer& composer, RenderContext* context) {}

// void PulseS2PSrclk(uint8_t count) {
//   digitalWrite(S2P_SRCLK, false);
//   for (uint8_t i = 0; i < count; i++) {
//     digitalWrite(S2P_SRCLK, true);
//     delayMicroseconds(1);
//     digitalWrite(S2P_SRCLK, false);
//     delayMicroseconds(1);
//   }
// }

void SetRedLed(bool state) {
  digitalWrite(RED_LED, state);
}

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
const uint8_t kNwWidth = 16;
const uint8_t kNwHeight = 8;
const uint8_t kMiddleMargin = 1;
const uint8_t kMiddleGap = 2;
const EePoint kHomePoint = EePoint(1 /*col*/, 1 /*row*/);
const EePoint kDebugPoint = EePoint(1 /*col*/, 30 /*row*/);

const uint8_t kNeWidth = kScreenWidth - kNwWidth - kLeftMargin - kMiddleMargin;
const uint8_t kNeHeight = kNwHeight;

const uint8_t kSwWidth = kNwWidth;
const uint8_t kSwHeight = (kScreenHeight - (kTopMargin + kMiddleGap + kNwHeight)) - 1;

const uint8_t kSeWidth = kNeWidth;
const uint8_t kSeHeight = kSwHeight;

// Using char* const was -2B RAM, +30B Flash
// char const kHbtValues[] = {'-', '\\', '|', '/'};
// char kHbtString[2];
char* const kHbtValues[] = {"-", "\\", "|", "/"};
uint8_t hbt_index = 0;
#define MEM_SIZE 128
uint8_t kenbakMemory[MEM_SIZE];
uint8_t memory_index;

FormatData ram_room_data = FormatData(uint16_t(0));
LogMessage ram_room_log = {kRoomStrings, &ram_room_data};

int8_t locale_string_delta(const char* const* locale_strings, uint8_t prev, uint8_t current) {
  if (prev_language != language) {
    uint8_t prev_width = TextWidth(locale_strings[prev]);
    uint8_t new_width = TextWidth(locale_strings[current]);
    return prev_width - new_width;
  } else {
    return 0;
  }
}

void app_nw(const EeComposer& composer, RenderContext* context) {
  composer.MoveTo(context->origin);
  composer.ComposeStringC(kSettingsStrings[language]);
  int8_t delta = locale_string_delta(kSettingsStrings, prev_language, language);
  if (delta > 0) {
    composer.ClearChars(delta);
  }
}

// bool print_help = false;
void app_ne(const EeComposer& composer, RenderContext* context) {
  composer.MoveTo(context->origin);
  composer.ComposeStringC(kTimeStrings[language]);
  composer.stream_->Write(0x20);
  composer.stream_->WriteDWord(uptime_ms);

  int8_t delta = locale_string_delta(kTimeStrings, prev_language, language);
  if (delta > 0) {
    composer.ClearChars(delta);
  }
}

EeFrame nw_frame = EeFrame(EePoint(kHomePoint.col() + kLeftMargin, kHomePoint.row() + kTopMargin),
                           kNwWidth, kNwHeight, app_nw);
EeFrame ne_frame =
    EeFrame(EePoint(nw_frame.origin().col() + kNwWidth + kMiddleMargin, nw_frame.origin().row()),
            kNeWidth, kNeHeight, app_ne);

const EePoint kInputOrigin =
    EePoint(ne_frame.origin().col() + 1, kTopMargin + kNwHeight + kMiddleGap);

EeText hbt_text = EeText(EePoint(ne_frame.origin().col() - 1, kTopMargin + kNwHeight + kMiddleGap));

EeText input_text = EeText(kInputOrigin);
EeText status_text = EeText(EePoint(kLeftMargin + 2, hbt_text.origin().row()));

void ComposeNW() {

  nw_frame.Compose(composer);
  // settings_text.Compose(composer);
}
void ComposeNE() {
  ne_frame.Compose(composer);
}

void ComposeSheet() {
  composer.ShowCursor(false);
  ComposeNW();
  ComposeNE();
  status_text.Compose(composer);
  input_text.Compose(composer);
  composer.MoveTo(input_text.origin() + EePoint(input_count == 0 ? 0 : 1 /*col*/, 0 /*row*/));
  composer.ShowCursor(true);
}

CmdProcessorState cmd_state;

int ReadCommand(uint8_t* character) {
  char byte = console_uart.ReadByte();
  if (byte < 0x20) {
    return -1;
  }
  *character = byte;
  return 0;
}

void print_debug() {
  composer.MoveTo(kDebugPoint);
  composer.ComposeStringC("Ram used: ");
  composer.stream_->WriteWord(kRamUsed);
  composer.ComposeStringC("\r\nFlash used: ");
  composer.stream_->WriteWord(kFlashUsed);
  composer.ComposeStringC("\r\nRoom: ");
  composer.stream_->WriteWord(ram_room_log.data->data.word);
#define WORD ram_room_log.data->data.word
  uint8_t post_clear = 0;
  // Only 512B ram, so only need to check up to 3 digits
  if (WORD < 100) {
    post_clear++;
    if (WORD < 10) {
      post_clear++;
    }
  }
  composer.ClearChars(post_clear);
}

// How many ticks (today, 1tick = 1ms) until we run
//  >0 = decrement each tick
//  0 = ready to run
//  <0 = do not update counter each tick
#define HBT_TASK_PERIOD 200
int16_t hbt_task_counter;

uint16_t last_millis;
const char* active_cmd;
uint8_t cmd_idx;
void setup() {
  hbt_task_counter = HBT_TASK_PERIOD;
  memory_index = 0;

  console_uart.Configure(115200);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  // pinMode(P2S_SH_LDL, OUTPUT);
  // pinMode(P2S_CLK, OUTPUT);
  // pinMode(P2S_OUT, INPUT);

  // pinMode(S2P_SER, OUTPUT);
  // pinMode(S2P_SRCLK, OUTPUT);
  // pinMode(S2P_RCLK, OUTPUT);

  input_text.SetText("");
  status_text.SetText(kReadyStrings[language]);

  hbt_text.SetText(kHbtValues[hbt_index]);
  composer.ClearScreen();

  ComposeSheet();

  volatile unsigned int sp;
  __asm__("MOV R1, %0" : "=r"(sp));
  unsigned int idx;
  for (idx = sp; idx > stack_end; idx -= 2) {
    *((unsigned int*)idx) = STACK_PAINT_COLOR;
  }

  last_millis = millis();
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
  // kHbtString[0] = kHbtValues[hbt_index];
  hbt_text.SetText(kHbtValues[hbt_index]);
  // digitalWrite(GREEN_LED, hbt_index & 0x1);

  // Check stack
  int idx;
  volatile unsigned int sp;
  __asm__("MOV R1, %0" : "=r"(sp));
  for (idx = sp; idx > stack_end; idx -= 2) {
    if (*((unsigned int*)idx) == STACK_PAINT_COLOR) {
      break;
    }
  }
  ram_room_log.data->data.word = idx - stack_end;

  composer.ShowCursor(false);
  hbt_text.Compose(composer);
  composer.MoveTo(input_text.origin() + EePoint(input_count == 0 ? 0 : 1 /*col*/, 0 /*row*/));
  composer.ShowCursor(true);

  kenbakMemory[memory_index++] = hbt_index;
  if (memory_index > MEM_SIZE - 1) {
    memory_index = 0;
  }
  print_debug();
}

uint16_t ms_since_last_check(uint16_t* last_millis) {
  uint16_t new_millis = millis();
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
  // static RawCommand cmd;
  uint8_t cmd;

  delay(1);
  uptime_ms += ms_since_last_check(&last_millis);

  // Always check for data
  if (console_uart.Available()) {

    if ((-1 != ReadCommand(&cmd))) {

      if (cmd != ' ') {
        input_text.Relocate(kInputOrigin + EePoint(input_count, 0 /*row*/));
        input_count++;
        input_text.SetText((char*)&cmd);
      }
      switch (cmd_state) {
        case kReady:
          if (cmd == kCmdHelp[cmd_idx]) {
            cmd_state = kReceiving;
            active_cmd = kCmdHelp;
          } else if (cmd == kCmdLangEn[cmd_idx]) {
            cmd_state = kReceiving;
            active_cmd = kCmdLangEn;
          } else if (cmd == kCmdLangRu[cmd_idx]) {
            cmd_state = kReceiving;
            active_cmd = kCmdLangRu;
          } else {
            active_cmd = 0;
          }
          break;
        case kReceiving:
          if (cmd == ' ') {
            break;
          }
          cmd_idx++;
          if ((0 == active_cmd[cmd_idx]) || (cmd != active_cmd[cmd_idx])) {
            // gone past the command or mismatched
            cmd_state = kDone;
            active_cmd = 0;
          }
          break;
        case kDone:
          break;
      }
    } else {
      digitalWrite(GREEN_LED, debug_state ^= 0x1);
      // Saw a frame start after receiving text
      input_text.Relocate(kInputOrigin);
      composer.MoveTo(kInputOrigin);
      composer.ClearChars(input_count);
      input_text.SetText("");

      bool valid_cmd = false;
      if (active_cmd == kCmdLangEn) {
        language = LANG_EN;
        valid_cmd = true;
      } else if (active_cmd == kCmdLangRu) {
        language = LANG_RU;
        valid_cmd = true;
      }
      if (valid_cmd) {
        status_text.SetText(kAckStrings[language]);
      } else {
        status_text.SetText(kErrorStrings[language]);
      }

      cmd_state = kReady;
      input_count = 0;
      cmd_idx = 0;
    }

    // Draw
    SetRedLed(true);
    ComposeSheet();
    SetRedLed(false);

    prev_language = language;
  }

  if (hbt_task_counter > 0) {
    hbt_task_counter--;
  } else if (hbt_task_counter == 0) {
    task_hbt();
    hbt_task_counter = HBT_TASK_PERIOD;
  }
}
