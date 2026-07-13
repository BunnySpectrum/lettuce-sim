#pragma once

#include <stdint.h>

#define LANG_EN 0
extern const char* const kEnTime;
extern const char* const kEnSettings;
extern const char* const kEnReady;
extern const char* const kEnEnterCmdDone;
extern const char* const kEnStack;
extern const char* const kEnRoom;
extern const char* const kEnHelp;
extern const char* const kEnError;

#define LANG_RU 1
extern const char* const kRuTime;
extern const char* const kRuSettings;
extern const char* const kRuReady;
extern const char* const kRuEnterCmdDone;
extern const char* const kRuStack;
extern const char* const kRuRoom;
extern const char* const kRuHelp;
extern const char* const kRuError;

// XXX may turn to enumerated list per language
// would need some scripting/codegen tho
extern const char* const kTimeStrings[2];
extern const char* const kSettingsStrings[2];
extern const char* const kReadyStrings[2];
extern const char* const kAckStrings[2];
extern const char* const kStackStrings[2];
extern const char* const kRoomStrings[2];
extern const char* const kHelpStrings[2];
extern const char* const kErrorStrings[2];

uint8_t TextWidth(const char* text);
