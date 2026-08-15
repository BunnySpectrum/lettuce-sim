#ifndef CMD_PROCESSOR_H
#define CMD_PROCESSOR_H
#include <stdint.h>

enum CmdProcessorState {
  kReady = 0,  // ready for new command
  kToWhere,    // looking for destination
  kReceiving,  // send characters to dest
  kDone,       // done receiving for this command
};

enum class Command : uint8_t {
  kUnknown,
  kHelp,
  kLanguage,
};

#endif