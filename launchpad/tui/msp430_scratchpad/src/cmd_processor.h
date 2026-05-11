
enum CmdProcessorState {
  kReady = 0,  // ready for new command
  kToWhere,    // looking for destination
  kReceiving,  // send characters to dest
  kDone,       // done receiving for this command
};

const char* const kCmdHelp = "help";
const char* const kCmdLangEn = "en";
const char* const kCmdLangRu = "ru";
char const kSetLangEn[] = "e";
char const kSetLangRu[] = "r";