#ifndef CPU_H
#define CPU_H
#include <stdint.h>
#undef OUTPUT
#undef INPUT

enum class CodeAddressing : uint8_t {
  kConstant = 3,
  kMemory = 4,
  kIndirect = 5,
  kIndexed = 6,
  kIndiredtIndexed = 7,
};

enum class CodeComparison : uint8_t {
  kNotEqualZero = 3,
  kEqualZero = 4,
  kLessThanZero = 5,
  kGreaterEqualZero = 6,
  kGreaterZero = 7,
};

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

#define MEM_SIZE 256
class Kenbak {

 public:
  Kenbak() : step_(false), cursorAddress_(0) {
    memory[0003] = 0004;
    memory[0004] = 0103;
    memory[0005] = 0001;
    memory[0006] = 0134;
    memory[0007] = 0200;
    memory[0010] = 0344;
    memory[0011] = 0004;
    // Add A
    memory[0020] = 0003;  // Constant
    memory[0021] = 0004;  // Memory
    memory[0022] = 0005;  // Indirect
    memory[0023] = 0006;  // Indexed
    memory[0024] = 0007;  // IndirectIndexed

    // Add B
    memory[0025] = 0103;  // Constant
    memory[0026] = 0104;  // Memory
    memory[0027] = 0105;  // Indirect
    memory[0030] = 0106;  // Indexed
    memory[0031] = 0107;  // IndirectIndexed

    // Add X
    memory[0032] = 0203;  // Constant
    memory[0033] = 0204;  // Memory
    memory[0034] = 0205;  // Indirect
    memory[0035] = 0206;  // Indexed
    memory[0036] = 0207;  // IndirectIndexed

    // Sub A
    memory[0037] = 0013;  // Constant
    memory[0040] = 0014;  // Memory
    memory[0041] = 0015;  // Indirect
    memory[0042] = 0016;  // Indexed
    memory[0043] = 0017;  // IndirectIndexed

    // Sub B
    memory[0044] = 0113;  // Constant
    memory[0045] = 0114;  // Memory
    memory[0046] = 0115;  // Indirect
    memory[0047] = 0116;  // Indexed
    memory[0050] = 0117;  // IndirectIndexed

    // Sub X
    memory[0051] = 0213;  // Constant
    memory[0052] = 0214;  // Memory
    memory[0053] = 0215;  // Indirect
    memory[0054] = 0216;  // Indexed
    memory[0055] = 0217;  // IndirectIndexed

    // Load A
    memory[0056] = 0023;  // Constant
    memory[0057] = 0024;  // Memory
    memory[0060] = 0025;  // Indirect
    memory[0061] = 0026;  // Indexed
    memory[0062] = 0027;  // IndirectIndexed

    // Load B
    memory[0063] = 0123;  // Constant
    memory[0064] = 0124;  // Memory
    memory[0065] = 0125;  // Indirect
    memory[0066] = 0126;  // Indexed
    memory[0067] = 0127;  // IndirectIndexed

    // Load X
    memory[0070] = 0223;  // Constant
    memory[0071] = 0224;  // Memory
    memory[0072] = 0225;  // Indirect
    memory[0073] = 0226;  // Indexed
    memory[0074] = 0227;  // IndirectIndexed

    // Store A
    memory[0075] = 0033;  // Constant
    memory[0076] = 0034;  // Memory
    memory[0077] = 0035;  // Indirect
    memory[0100] = 0036;  // Indexed
    memory[0101] = 0037;  // IndirectIndexed

    // Store B
    memory[0102] = 0133;  // Constant
    memory[0103] = 0134;  // Memory
    memory[0104] = 0135;  // Indirect
    memory[0105] = 0136;  // Indexed
    memory[0106] = 0137;  // IndirectIndexed

    // Store X
    memory[0107] = 0233;  // Constant
    memory[0110] = 0234;  // Memory
    memory[0111] = 0235;  // Indirect
    memory[0112] = 0236;  // Indexed
    memory[0113] = 0237;  // IndirectIndexed

    // Or
    memory[0114] = 0303;  // Constant
    memory[0115] = 0304;  // Memory
    memory[0116] = 0305;  // Indirect
    memory[0117] = 0306;  // Indexed
    memory[0120] = 0307;  // IndirectIndexed

    // And
    memory[0121] = 0323;  // Constant
    memory[0122] = 0324;  // Memory
    memory[0123] = 0325;  // Indirect
    memory[0124] = 0326;  // Indexed
    memory[0125] = 0327;  // IndirectIndexed

    // Lneg
    memory[0126] = 0333;  // Constant
    memory[0127] = 0334;  // Memory
    memory[0130] = 0335;  // Indirect
    memory[0131] = 0336;  // Indexed
    memory[0132] = 0337;  // IndirectIndexed

    // JPD A
    memory[0133] = 0043;  // NotEqualZero
    memory[0134] = 0044;  // EqualZero
    memory[0135] = 0045;  // LessThanZero
    memory[0136] = 0046;  // GreaterEqualZero
    memory[0137] = 0047;  // GreaterThanZero

    // JPD B
    memory[0140] = 0143;  // NotEqualZero
    memory[0141] = 0144;  // EqualZero
    memory[0142] = 0145;  // LessThanZero
    memory[0143] = 0146;  // GreaterEqualZero
    memory[0144] = 0147;  // GreaterThanZero

    // JPD X
    memory[0145] = 0243;  // NotEqualZero
    memory[0146] = 0244;  // EqualZero
    memory[0147] = 0245;  // LessThanZero
    memory[0150] = 0246;  // GreaterEqualZero
    memory[0151] = 0247;  // GreaterThanZero

    // JPD Unconditional
    memory[0152] = 0343;  // NotEqualZero
    memory[0153] = 0344;  // EqualZero
    memory[0154] = 0345;  // LessThanZero
    memory[0155] = 0346;  // GreaterEqualZero
    memory[0156] = 0347;  // GreaterThanZero

    // JPI A
    memory[0157] = 0053;  // NotEqualZero
    memory[0160] = 0054;  // EqualZero
    memory[0161] = 0055;  // LessThanZero
    memory[0162] = 0056;  // GreaterEqualZero
    memory[0163] = 0057;  // GreaterThanZero

    // JPI B
    memory[0164] = 0153;  // NotEqualZero
    memory[0165] = 0154;  // EqualZero
    memory[0166] = 0155;  // LessThanZero
    memory[0167] = 0156;  // GreaterEqualZero
    memory[0170] = 0157;  // GreaterThanZero

    // JPI X
    memory[0171] = 0253;  // NotEqualZero
    memory[0172] = 0254;  // EqualZero
    memory[0173] = 0255;  // LessThanZero
    memory[0174] = 0256;  // GreaterEqualZero
    memory[0175] = 0257;  // GreaterThanZero

    // JPI Unconditional
    memory[0176] = 0353;  // NotEqualZero
    memory[0177] = 0354;  // EqualZero
    memory[0200] = 0355;  // LessThanZero
    memory[0201] = 0356;  // GreaterEqualZero
    memory[0202] = 0357;  // GreaterThanZero

    // JMD A
    memory[0203] = 0063;  // NotEqualZero
    memory[0204] = 0064;  // EqualZero
    memory[0205] = 0065;  // LessThanZero
    memory[0206] = 0066;  // GreaterEqualZero
    memory[0207] = 0067;  // GreaterThanZero

    // JMD B
    memory[0210] = 0163;  // NotEqualZero
    memory[0211] = 0164;  // EqualZero
    memory[0212] = 0165;  // LessThanZero
    memory[0213] = 0166;  // GreaterEqualZero
    memory[0214] = 0167;  // GreaterThanZero

    // JMD X
    memory[0215] = 0263;  // NotEqualZero
    memory[0216] = 0264;  // EqualZero
    memory[0217] = 0265;  // LessThanZero
    memory[0220] = 0266;  // GreaterEqualZero
    memory[0221] = 0267;  // GreaterThanZero

    // JMD Unconditional
    memory[0222] = 0363;  // NotEqualZero
    memory[0223] = 0364;  // EqualZero
    memory[0224] = 0365;  // LessThanZero
    memory[0225] = 0366;  // GreaterEqualZero
    memory[0226] = 0367;  // GreaterThanZero

    // JMI A
    memory[0227] = 0073;  // NotEqualZero
    memory[0230] = 0074;  // EqualZero
    memory[0231] = 0075;  // LessThanZero
    memory[0232] = 0076;  // GreaterEqualZero
    memory[0233] = 0077;  // GreaterThanZero

    // JMI B
    memory[0234] = 0173;  // NotEqualZero
    memory[0235] = 0174;  // EqualZero
    memory[0236] = 0175;  // LessThanZero
    memory[0237] = 0176;  // GreaterEqualZero
    memory[0240] = 0177;  // GreaterThanZero

    // JMI X
    memory[0241] = 0273;  // NotEqualZero
    memory[0242] = 0274;  // EqualZero
    memory[0243] = 0275;  // LessThanZero
    memory[0244] = 0276;  // GreaterEqualZero
    memory[0245] = 0277;  // GreaterThanZero

    // JMI Unconditional
    memory[0246] = 0373;  // NotEqualZero
    memory[0247] = 0374;  // EqualZero
    memory[0250] = 0375;  // LessThanZero
    memory[0251] = 0376;  // GreaterEqualZero
    memory[0252] = 0377;  // GreaterThanZero

        // Right shift A
    memory[0253] = 0011;  // 1
    memory[0254] = 0021;  // 2
    memory[0255] = 0031;  // 3
    memory[0256] = 0001;  // 4

    // Right shift B
    memory[0257] = 0051;  // 1
    memory[0260] = 0061;  // 2
    memory[0261] = 0071;  // 3
    memory[0262] = 0041;  // 4

    // Right rotate A
    memory[0263] = 0111;  // 1
    memory[0264] = 0121;  // 2
    memory[0265] = 0131;  // 3
    memory[0266] = 0101;  // 4

    // Right rotate B
    memory[0267] = 0151;  // 1
    memory[0270] = 0161;  // 2
    memory[0271] = 0171;  // 3
    memory[0272] = 0141;  // 4

    // Left shift A
    memory[0273] = 0211;  // 1
    memory[0274] = 0221;  // 2
    memory[0275] = 0231;  // 3
    memory[0276] = 0201;  // 4

    // Left shift B
    memory[0277] = 0251;  // 1
    memory[0300] = 0261;  // 2
    memory[0301] = 0271;  // 3
    memory[0302] = 0241;  // 4

    // Left rotate A
    memory[0303] = 0311;  // 1
    memory[0304] = 0321;  // 2
    memory[0305] = 0331;  // 3
    memory[0306] = 0301;  // 4

    // Left rotate B
    memory[0307] = 0351;  // 1
    memory[0310] = 0361;  // 2
    memory[0311] = 0371;  // 3
    memory[0312] = 0341;  // 4

    // Halt
    memory[0313] = 0000;

    // Noop
    memory[0314] = 0200;
  }

 static bool is_add_sub_load_store(uint8_t value) {
  return ((value & 0007) > 0002) && ((value & 0070) < 0040) && ((value & 0700) < 0300);
}

  void toggle_step() { step_ ^= true; }
  void move_cursor_address(int16_t amount) { cursorAddress_ += amount; }
  uint8_t memory_read(uint8_t address) const { return memory[address]; }
  uint8_t register_read(KenbakReg reg) const { return memory[static_cast<uint8_t>(reg)]; }
  void execute() { memory[static_cast<uint8_t>(KenbakReg::PC)]++; }

  void register_write(KenbakReg reg, uint8_t value) { memory[static_cast<uint8_t>(reg)] = value; }
  void memory_write(uint8_t address, uint8_t value) { memory[address] = value; }

  uint8_t cursor_address() const { return cursorAddress_; }
  bool step() const { return step_; }
  uint8_t memory[MEM_SIZE];

 private:
  //   uint8_t memory_[MEM_SIZE];
  uint8_t cursorAddress_;
  bool step_;
};
#undef MEM_SIZE
#endif