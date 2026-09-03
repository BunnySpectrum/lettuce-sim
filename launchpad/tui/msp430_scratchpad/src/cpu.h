#ifndef CPU_H
#define CPU_H
#include <stdint.h>
#include <string.h>
#include "ee_composer.h"
#undef OUTPUT
#undef INPUT


enum class Operation : uint8_t {
    kUnknown = 0,
    kHalt,
    kNoop,
    kRShift,
    kRRotate,
    kLShift,
    kLRotate,
    kSet0,
    kSet1,
    kSkip0,
    kSkip1,
    kOr,
    kAnd,
    kLoadNeg,
    kAdd,
    kSub,
    kLoad,
    kStore,
    kJumpDirect,
    kJumpIndirect,
    kJumpMarkDirect,
    kJumpMarkIndirect,
    kCount
};
extern const char* const kOperationNames[static_cast<uint8_t>(Operation::kCount)];

enum class CodeAddressing : uint8_t {
  kConstant = 3,
  kMemory = 4,
  kIndirect = 5,
  kIndexed = 6,
  kIndirectIndexed = 7,

  kBegin = kConstant,
  kEnd = kIndirectIndexed,
  kCount = (kEnd - kBegin)+1,
};
inline CodeAddressing& operator++(CodeAddressing& obj){
  obj = static_cast<CodeAddressing>(static_cast<uint8_t>(obj) + 1);
  return obj;
}
extern const char* const kAddressingNames[static_cast<uint8_t>(CodeAddressing::kCount)];

enum class OpReg : uint8_t {
  kA = 0,
  kB = 1,
  kX = 2,

  kBegin = kA,
  kEnd = kX,
  kCount = (kEnd - kBegin)+1,
};
inline OpReg& operator++(OpReg& obj){
  obj = static_cast<OpReg>(static_cast<uint8_t>(obj) + 1);
  return obj;
}
extern const char* const kOpRegNames[static_cast<uint8_t>(OpReg::kCount)];

enum class OpJumpTest : uint8_t {
  kA = 0,
  kB = 1,
  kX = 2,
  kUnconditional = 3,
  
  kBegin = kA,
  kEnd = kUnconditional,
  kCount = (kEnd - kBegin)+1,
};
inline OpJumpTest& operator++(OpJumpTest& obj){
  obj = static_cast<OpJumpTest>(static_cast<uint8_t>(obj) + 1);
  return obj;
}
extern const char* const kOpJumpTestNames[static_cast<uint8_t>(OpJumpTest::kCount)];

enum class OpShiftReg : uint8_t {
  kA = 0,
  kB = 1,

  kBegin = kA,
  kEnd = kB,
  kCount = (kEnd - kBegin)+1,
};
inline OpShiftReg& operator++(OpShiftReg& obj){
  obj = static_cast<OpShiftReg>(static_cast<uint8_t>(obj) + 1);
  return obj;
}
extern const char* const kOpShiftRegNames[static_cast<uint8_t>(OpShiftReg::kCount)];

enum class CodeComparison : uint8_t {
  kNotEqualZero = 3,
  kEqualZero = 4,
  kLessThanZero = 5,
  kGreaterEqualZero = 6,
  kGreaterZero = 7,
  
  kBegin = kNotEqualZero,
  kEnd = kGreaterZero,
  kCount = (kEnd - kBegin)+1,
};
inline CodeComparison& operator++(CodeComparison& obj){
  obj = static_cast<CodeComparison>(static_cast<uint8_t>(obj) + 1);
  return obj;
}
extern const char* const kComparisonNames[static_cast<uint8_t>(CodeComparison::kCount)];





enum class CodeGroup : uint8_t {
  kAddSubLoadStore = 0,
  kOrAndLneg,
  kJumps,
  kBits,
  kShiftRotate,
  kMisc,
};

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

size_t write_operand(EeComposer composer, Operation operation, uint8_t operand, CodeAddressing addressing);

struct OpBase{
  Operation operation;

  virtual size_t write(EeComposer composer) = 0;
  virtual void destroy() = 0;

 protected:
  ~OpBase() {}
};

struct OpAddSubLoadStore : OpBase{
  OpReg reg;
  CodeAddressing addressing;
  uint8_t operand;

  size_t write(EeComposer composer){
    size_t wrote = 0;
    wrote += composer.ComposeStringC(kOperationNames[static_cast<uint8_t>(operation)]);
    wrote += composer.ComposeStringC(" ");
    wrote += composer.ComposeStringC(kOpRegNames[static_cast<uint8_t>(reg) - static_cast<uint8_t>(OpReg::kBegin)]);
    wrote += composer.ComposeStringC(" ");
    wrote += write_operand(composer, operation, operand, addressing);
    return wrote;
  }

  void destroy() { this->~OpAddSubLoadStore(); }

  OpAddSubLoadStore(Operation op, uint8_t first, uint8_t second){
    const uint8_t kUpper = (first & 0700) >> 6;
    const uint8_t kLower = first & 0007;
    operation = op;
    reg = static_cast<OpReg>(kUpper);
    addressing = static_cast<CodeAddressing>(kLower);
    operand = second;
  }
};

struct OpOrAndLneg : OpBase{
  CodeAddressing addressing;
  uint8_t operand;
  
  size_t write(EeComposer composer){
    size_t wrote = 0;
    wrote += composer.ComposeStringC(kOperationNames[static_cast<uint8_t>(operation)]);
    wrote += composer.ComposeStringC(" ");
    wrote += write_operand(composer, operation, operand, addressing);
    return wrote;
  }
  void destroy() { this->~OpOrAndLneg(); }

  OpOrAndLneg(Operation op, uint8_t first, uint8_t second){
    const uint8_t kLower = first & 0007;
    operation = op;
    addressing = static_cast<CodeAddressing>(kLower);
    operand = second;
  }
};

struct OpJumps : OpBase{
  OpJumpTest testSource;
  CodeComparison comparison;
  uint8_t operand;
  
  size_t write(EeComposer composer){
    size_t wrote = 0;
    wrote += composer.ComposeStringC(kOperationNames[static_cast<uint8_t>(operation)]);
    wrote += composer.ComposeStringC(" ");
    wrote += composer.ComposeStringC(kOpJumpTestNames[static_cast<uint8_t>(testSource) - static_cast<uint8_t>(OpJumpTest::kBegin)]);
    if (testSource != OpJumpTest::kUnconditional){
      wrote += composer.ComposeStringC(kComparisonNames[static_cast<uint8_t>(comparison) - static_cast<uint8_t>(CodeComparison::kBegin)]);
    }
    wrote += composer.ComposeStringC(" ");

    if((operation == Operation::kJumpIndirect) || (operation == Operation::kJumpMarkIndirect)){
      wrote += composer.stream_->WriteChar('(');
      wrote += composer.stream_->WriteOct(operand);
      wrote += composer.stream_->WriteChar(')');
    }else{
      wrote += composer.stream_->WriteOct(operand);
    }
    return wrote;
  }
  void destroy() { this->~OpJumps(); }

  OpJumps(Operation op, uint8_t first, uint8_t second){
    const uint8_t kUpper = (first & 0700) >> 6;
    const uint8_t kLower = first & 0007;
    operation = op;
    testSource = static_cast<OpJumpTest>(kUpper);
    comparison = static_cast<CodeComparison>(kLower);
    operand = second;
  }
};

struct OpBits : OpBase{
  uint8_t digit;
  uint8_t operand;
  
  size_t write(EeComposer composer){
    size_t wrote = 0;
    wrote += composer.ComposeStringC(kOperationNames[static_cast<uint8_t>(operation)]);
    wrote += composer.ComposeStringC(" b");
    wrote += composer.stream_->WriteByte(digit);
    wrote += composer.ComposeStringC(" ");
    wrote += composer.stream_->WriteOct(operand);
    return wrote;
  }
  void destroy() { this->~OpBits(); }

  OpBits(Operation op, uint8_t first, uint8_t second){
    const uint8_t kMiddle = (first & 0070) >> 3;
    operation = op;
    digit = kMiddle;
    operand = second;
  }
};

struct OpShiftRotate : OpBase{
  OpShiftReg reg;
  uint8_t places;
  
  size_t write(EeComposer composer){
    size_t wrote = 0;
    wrote += composer.ComposeStringC(kOperationNames[static_cast<uint8_t>(operation)]);
    wrote += composer.ComposeStringC(" ");
    wrote += composer.ComposeStringC(kOpShiftRegNames[static_cast<uint8_t>(reg) - static_cast<uint8_t>(OpShiftReg::kBegin)]);
    wrote += composer.ComposeStringC(" ");
    wrote += composer.stream_->WriteByte(places);
    return wrote;
  }
  void destroy() { this->~OpShiftRotate(); }

  OpShiftRotate(Operation op, uint8_t first){
    const uint8_t kMiddle = (first & 0070) >> 3;
    operation = op;
    reg = kMiddle >= 4 ? OpShiftReg::kB : OpShiftReg::kA; 
    places = (kMiddle & 0x3) == 0 ? 4 : (kMiddle & 0x3);
  }
};

struct OpMisc : OpBase{
  size_t write(EeComposer composer){
    size_t wrote = 0;
    wrote += composer.ComposeStringC(kOperationNames[static_cast<uint8_t>(operation)]);
    return wrote;
  }
  void destroy() { this->~OpMisc(); }

  OpMisc(Operation op){
    operation = op;
  }
};


union KenbakInstruction{
  OpAddSubLoadStore addSubLoadStore;
  OpOrAndLneg orAndLneg;
  OpJumps jumps;
  OpBits bits;
  OpShiftRotate shiftRotate;
  OpMisc misc;
};

#define MEM_SIZE 256
extern const uint8_t kImageInstructions[MEM_SIZE];
extern const uint8_t kImageDemo[MEM_SIZE];
class Kenbak {

 public:
  Kenbak() : run_(false), cursorAddress_(0) {
    load_image(0);
  }

  void load_image(uint8_t slot){
    switch(slot){
      case 0:
        memcpy(memory, kImageInstructions, MEM_SIZE);
        break;
      case 1:
        memcpy(memory, kImageDemo, MEM_SIZE);
        break;
      default:
        break;
    }  
  }

 static bool decode_addressing(uint8_t value, CodeAddressing* addressing){
  const uint8_t kLower = value & 0007;
  for (auto candidate = CodeAddressing::kBegin; candidate <= CodeAddressing::kEnd; ++candidate){
    const uint8_t candidate_value = static_cast<uint8_t>(candidate);
    // if(static_cast<uint8_t>(candidate) == kLower){ zzz caused compiler segfault
    if(candidate_value == kLower){
      *addressing = candidate;
      return true;
    }
    // if(candidate == CodeAddressing::kEnd){
    //   break;
    // }
  }
  return false;
 }

 static CodeGroup decode_code_group(uint8_t value){
  const uint8_t kLower = value & 0007;
  const uint8_t kMiddle = (value & 0070) >> 3;
  const uint8_t kUpper = (value & 0700) >> 6;
  switch (kLower){
    case 0:
      return CodeGroup::kMisc;
    case 1:
      return CodeGroup::kShiftRotate;
    case 2:
      return CodeGroup::kBits;
  }
  if (kMiddle > 3){
    return CodeGroup::kJumps;
  }else if(kUpper == 3){
    return CodeGroup::kOrAndLneg;
  }else{
    return CodeGroup::kAddSubLoadStore;
  }
 }

 static Operation decode_operation(uint8_t value) {
  Operation result = Operation::kUnknown;
  const uint8_t kLower = value & 0007;
  const uint8_t kMiddle = (value & 0070) >> 3;
  const uint8_t kUpper = (value & 0700) >> 6;
  switch (kLower){
    case 0:
      result = value < 0200 ? Operation::kHalt : Operation::kNoop;
      break;
    case 1:
      switch (kUpper){
        case 0:
          result = Operation::kRShift;
          break;
        case 1:
          result = Operation::kRRotate;
          break;
        case 2:
          result = Operation::kLShift;
          break;
        case 3:
          result = Operation::kLRotate;
          break;
        default:
          break;
      }
      break;
    case 2:
      switch (kUpper){
        case 0:
          result = Operation::kSet0;
          break;
        case 1:
          result = Operation::kSet1;
          break;
        case 2:
          result = Operation::kSkip0;
          break;
        case 3:
          result = Operation::kSkip1;
          break;
        default:
          break;
      }
      break;
    default:
    // pivot on {jumps, {add/sub/load/store, or/and/lneg}}
      if (kMiddle > 3){
        // jump
        switch (kMiddle){
          case 4:
            result = Operation::kJumpDirect;
            break;
          case 5:
            result = Operation::kJumpIndirect;
            break;
          case 6:
            result = Operation::kJumpMarkDirect;
            break;
          case 7:
            result = Operation::kJumpMarkIndirect;
            break;
          default:
            break;
        }
      }else if(kUpper == 3){
        // or/and/lneg
        switch (kMiddle){
          case 0:
            result = Operation::kOr;
            break;
          case 1:
            result = Operation::kNoop;
            break;
          case 2:
            result = Operation::kAnd;
            break;
          case 3:
            result = Operation::kLoadNeg;
            break;
          default:
            break;
        }
      }else{
        // add/sub/load/store
        switch (kMiddle){
          case 0:
            result = Operation::kAdd;
            break;
          case 1:
            result = Operation::kSub;
            break;
          case 2:
            result = Operation::kLoad;
            break;
          case 3:
            result = Operation::kStore;
            break;
          default:
            break;
        }
      }
      break;
  }

  return result;
}


  void toggle_run() { run_ ^= true; }
  void move_cursor_address(int16_t amount) { cursorAddress_ += amount; }
  uint8_t memory_read(uint8_t address) const { return memory[address]; }
  uint8_t register_read(KenbakReg reg) const { return memory[static_cast<uint8_t>(reg)]; }
  void execute() { memory[static_cast<uint8_t>(KenbakReg::PC)]++; }

  void register_write(KenbakReg reg, uint8_t value) { memory[static_cast<uint8_t>(reg)] = value; }
  void memory_write(uint8_t address, uint8_t value) { memory[address] = value; }

  uint8_t cursor_address() const { return cursorAddress_; }
  bool is_running() const { return run_; }
  uint8_t memory[MEM_SIZE];

 private:
  uint8_t cursorAddress_;
  bool run_;
};
// #undef MEM_SIZE
#endif
