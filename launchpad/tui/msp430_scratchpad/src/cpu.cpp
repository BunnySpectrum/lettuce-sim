#include "cpu.h"

const char* const kOperationNames[static_cast<uint8_t>(Operation::kCount)] = {
    "???", 
    "HALT",
    "NO-OP",
    "SFTR",
    "ROTR",
    "SFTL",
    "ROTL",
    "SET0",
    "SET1",
    "SKP0",
    "SKP1",
    "OR",
    "AND",
    "LNEG",
    "ADD",
    "SUB",
    "LOAD",
    "STORE",
    "JPD",
    "JPI",
    "JMD",
    "JMI",
};

const char* const kAddressingNames[static_cast<uint8_t>(CodeAddressing::kCount)] = {
    "Imm",
    "Mem",
    "Indi",
    "Idx",
    "Indi/Idx",
};

const char* const kOpRegNames[static_cast<uint8_t>(OpReg::kCount)] = {
    "A",
    "B",
    "X",
};
const uint8_t kOpRegAddresses[static_cast<uint8_t>(OpReg::kCount)] = {
    static_cast<uint8_t>(KenbakReg::A),
    static_cast<uint8_t>(KenbakReg::B),
    static_cast<uint8_t>(KenbakReg::X),
};
const uint8_t kOpRegStatusAddresses[static_cast<uint8_t>(OpReg::kCount)] = {
    static_cast<uint8_t>(KenbakReg::AOC),
    static_cast<uint8_t>(KenbakReg::BOC),
    static_cast<uint8_t>(KenbakReg::XOC),
};

const char* const kOpJumpTestNames[static_cast<uint8_t>(OpJumpTest::kCount)] = {
    "A",
    "B",
    "X",
    "UNC",
};

const char* const kOpShiftRegNames[static_cast<uint8_t>(OpShiftReg::kCount)] = {
    "A",
    "B",
};

const char* const kComparisonNames[static_cast<uint8_t>(CodeComparison::kCount)] = {
    "!=0",
    "=0",
    "<0",
    ">=0",
    ">0",
};
const uint8_t kImageInstructions[MEM_SIZE] = {
    0000,0000,0000,0004,0200,0003,0004,0005,0006,0007,0103,0104,0105,0106,0107,0203,
    0204,0205,0206,0207,0013,0014,0015,0016,0017,0113,0114,0115,0116,0117,0213,0214,
    0215,0216,0217,0023,0024,0025,0026,0027,0123,0124,0125,0126,0127,0223,0224,0225,
    0226,0227,0033,0034,0035,0036,0037,0133,0134,0135,0136,0137,0233,0234,0235,0236,
    0237,0303,0304,0305,0306,0307,0323,0324,0325,0326,0327,0333,0334,0335,0336,0337,
    0043,0044,0045,0046,0047,0143,0144,0145,0146,0147,0243,0244,0245,0246,0247,0343,
    0344,0345,0346,0347,0053,0054,0055,0056,0057,0153,0154,0155,0156,0157,0253,0254,
    0255,0256,0257,0353,0354,0355,0356,0357,0063,0064,0065,0066,0067,0163,0164,0165,
    0000,0000,0000,0000,0166,0167,0263,0264,0265,0266,0267,0363,0364,0365,0366,0367,
    0073,0074,0075,0076,0077,0173,0174,0175,0176,0177,0273,0274,0275,0276,0277,0373,
    0374,0375,0376,0377,0002,0012,0022,0032,0042,0052,0062,0072,0102,0112,0122,0132,
    0142,0152,0162,0172,0202,0212,0222,0232,0242,0252,0262,0272,0302,0312,0322,0332,
    0342,0352,0362,0372,0011,0021,0031,0001,0051,0061,0071,0041,0111,0121,0131,0101,
    0151,0161,0171,0141,0211,0221,0231,0201,0251,0261,0271,0241,0311,0321,0331,0301,
    0351,0361,0371,0341,0000,0200,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,
    0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,
};
const uint8_t kImageDemo[MEM_SIZE] = {
    0000,0002,0003,0004,0004,0001,0023,0000,0005,0001,0000,0000,0000,0000,0000,0000,
    0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,
    0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,
    0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,
    0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,
    0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,
    0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,
    0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,
    0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,
    0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,
    0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,
    0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,
    0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,
    0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,
    0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,
    0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,
};


size_t write_operand(EeComposer composer, Operation operation, uint8_t operand, CodeAddressing addressing){
  size_t wrote = 0;
  switch (addressing){
    case CodeAddressing::kConstant:
      wrote += composer.ComposeStringC("C=");
      if (operation != Operation::kStore){
        wrote += composer.stream_->WriteOct(operand);
      }
      break;
    
    case CodeAddressing::kMemory:
      wrote += composer.stream_->WriteOct(operand);
      break;

    case CodeAddressing::kIndirect:
      wrote += composer.stream_->WriteChar('(');
      wrote += composer.stream_->WriteOct(operand);
      wrote += composer.stream_->WriteChar(')');
      break;

    case CodeAddressing::kIndexed:
      wrote += composer.stream_->WriteOct(operand);
      wrote += composer.ComposeStringC(",X");
      break;
    case CodeAddressing::kIndirectIndexed:
      wrote += composer.stream_->WriteChar('(');
      wrote += composer.stream_->WriteOct(operand);
      wrote += composer.ComposeStringC("),X");
      break;
    default:
      wrote += composer.stream_->WriteOct(operand);
      break;
  }
  return wrote;
}