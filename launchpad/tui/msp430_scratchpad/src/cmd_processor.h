
enum CmdProcessorState {
  kReady = 0,  // ready for new command
  kToWhere,    // looking for destination
  kReceiving,  // send characters to dest
  kDone,       // done receiving for this command
};

const char* const kCmdHelp = "help";
char const kSetLangEn[] = "e";
char const kSetLangRu[] = "r";