

#include "locale.h"
const char* const kEnTime = "Time";
const char* const kRuTime = "\xd0\xa7\xd0\x90\xd0\xa1";
// ЧАС

const char* const kEnSettings = "Settings";
const char* const kRuSettings =
    "\xd0\x9d\xd0\x90\xd0\xa1\xd0\xa2\xd0\xa0\xd0\x9e\xd0\x99\xd0\x9a\xd0\x98";
// НАСТРОЙКИ

const char* const kEnReady = "Ready";
const char* const kRuReady = "\xd0\x93\xd0\x9e\xd0\xa2\xd0\x9e\xd0\x92\xd0\x90\xd0\xaf";
// ГОТОВАЯ

const char* const kEnAck = "ACK";
const char* const kRuAck =
    "\xd0\x9f\xd0\x9e\xd0\x94\xd0\xa2\xd0\x92\xd0\x95\xd0\xa0\xd0\x96\xd0\x94\xd0\x95\xd0\x9d";
// ПОДТВЕРЖДЕН

const char* const kEnStack = "Stack";
const char* const kRuStack = "\xd0\xa1\xd0\xa2\xd0\x95\xd0\x9a";
//СТЕК

const char* const kEnRoom = "Room";
const char* const kRuRoom = "\xd0\x97\xd0\x90\xd0\x9f\xd0\x90\xd0\xa1";
// ЗАПАС

const char* const kEnHelp = "Help";
const char* const kRuHelp = "\xd0\xbf\xd0\xbe\xd0\xbc\xd0\xbe\xd1\x89\xd1\x8c";

const char* const kEnError = "Error";
const char* const kRuError = "\xd0\xbe\xd1\x88\xd0\xb8\xd0\xb1\xd0\xba\xd0\xb0";

const char* const kTimeStrings[] = {kEnTime, kRuTime};
const char* const kSettingsStrings[] = {kEnSettings, kRuSettings};
const char* const kReadyStrings[] = {kEnReady, kRuReady};
const char* const kAckStrings[] = {kEnAck, kRuAck};
const char* const kStackStrings[] = {kEnStack, kRuStack};
const char* const kRoomStrings[] = {kEnRoom, kRuRoom};
const char* const kHelpStrings[] = {kEnHelp, kRuHelp};
const char* const kErrorStrings[] = {kEnError, kRuError};

/*
U+uvwzyz
CP1: U+0000    -  U+007F:      0yyyxxxx
CP2: U+0080    -  U+07FF:      110xxxyy   10yyzzzz
CP3: U+0800    -  U+FFFF:      1110wwww   10xxxxyy   10yyxxxx
CP4: U+010000  -  U+10FFFF:    11110uvv   10vvwwww   10xxxxyy   10yyzzzz
*/

uint8_t TextWidth(const char* text) {
  uint8_t index = 0;
  uint8_t width = 0;
  char byte;
  bool is_multi_byte = false;

  do {
    byte = text[index];
    if (byte == 0) {
      break;
    }
    if ((byte & 0b10000000) == 0) {
      // Is first code point range
      width++;
    } else if ((byte & 0b11000000) == 0b11000000) {
      // start of UTF-8
      width++;
    } else {
      // should be byte 2, 3 , or 4
      // assert (byte & 0b11000000) == 0b10000000
    }
    index++;
  } while (byte != 0);

  return width;
}