from __future__ import annotations
import enum
from abc import ABC, abstractmethod


class Register(enum.Enum):
    kA = 0
    kB = 1
    kX = 2

    def c_name(self) -> str:
        return self.name[1:]

class RegisterShiftRotate(enum.Enum):
    kA = 0
    kB = 4

    def c_name(self) -> str:
        return self.name[1:]

class Addressing(enum.Enum):
    kConstant = 3
    kMemory = 4
    kIndirect = 5
    kIndexed = 6
    kIndirectIndexed = 7

    def c_name(self) -> str:
        return self.name[1:]


class JumpCheck(enum.Enum):
    kA = 0
    kB = 1
    kX = 2
    kUnconditional = 3

    def c_name(self) -> str:
        return self.name[1:]


class JumpCondition(enum.Enum):
    kNotEqualZero = 3
    kEqualZero = 4
    kLessThanZero = 5
    kGreaterEqualZero = 6
    kGreaterThanZero = 7

    def c_name(self) -> str:
        return self.name[1:]


class OctalByte:
    def __init__(self, value: int):
        if (value < 0) or (value > 255):
            raise ValueError(value)

        self._value = value

    def set_high(self, data: int):
        data &= 0o3
        data <<= 6
        self._value &= ~0o300
        self._value |= data

    def set_mid(self, data: int):
        data &= 0o7
        data <<= 3
        self._value &= ~0o070
        self._value |= data

    def set_low(self, data: int):
        data &= 0o007
        self._value &= ~0o007
        self._value |= data

    def value(self) -> int:
        return self._value


class OpGroupAddSubLoadStore(ABC):
    def __init__(self, register: Register, addressing: Addressing):
        self._register = register
        self._addressing = addressing

    def to_code(self) -> list[int]:
        code = OctalByte(0)
        code.set_high(self._register.value)
        code.set_mid(self.opcode())
        code.set_low(self._addressing.value)

        return [code.value()]

    @classmethod
    @abstractmethod
    def opcode(cls) -> int:
        pass

    @classmethod
    @abstractmethod
    def c_name(cls) -> str:
        pass


class OpAdd(OpGroupAddSubLoadStore):
    @classmethod
    def opcode(cls) -> int:
        return 0

    @classmethod
    def c_name(cls) -> str:
        return "Add"


class OpSub(OpGroupAddSubLoadStore):
    @classmethod
    def opcode(cls) -> int:
        return 1

    @classmethod
    def c_name(cls) -> str:
        return "Sub"


class OpLoad(OpGroupAddSubLoadStore):
    @classmethod
    def opcode(cls) -> int:
        return 2

    @classmethod
    def c_name(cls) -> str:
        return "Load"


class OpStore(OpGroupAddSubLoadStore):
    @classmethod
    def opcode(cls) -> int:
        return 3

    @classmethod
    def c_name(cls) -> str:
        return "Store"


class OpGroupOrAndLneg(ABC):
    def __init__(self, addressing: Addressing):
        self._addressing = addressing

    def to_code(self) -> list[int]:
        code = OctalByte(0)
        code.set_high(3)
        code.set_mid(self.opcode())
        code.set_low(self._addressing.value)

        return [code.value()]

    @classmethod
    @abstractmethod
    def opcode(cls) -> int:
        pass

    @classmethod
    @abstractmethod
    def c_name(cls) -> str:
        pass


class OpOr(OpGroupOrAndLneg):
    @classmethod
    def opcode(cls) -> int:
        return 0

    @classmethod
    def c_name(cls) -> str:
        return "Or"


class OpAnd(OpGroupOrAndLneg):
    @classmethod
    def opcode(cls) -> int:
        return 2

    @classmethod
    def c_name(cls) -> str:
        return "And"


class OpLneg(OpGroupOrAndLneg):
    @classmethod
    def opcode(cls) -> int:
        return 3

    @classmethod
    def c_name(cls) -> str:
        return "Lneg"


class OpGroupJump(ABC):
    def __init__(self, check: JumpCheck, condition: JumpCondition):
        self._check = check
        self._condition = condition

    def to_code(self) -> list[int]:
        code = OctalByte(0)
        code.set_high(self._check.value)
        code.set_mid(self.opcode())
        code.set_low(self._condition.value)

        return [code.value()]

    @classmethod
    @abstractmethod
    def opcode(cls) -> int:
        pass

    @classmethod
    @abstractmethod
    def c_name(cls) -> str:
        pass

class OpJumpDirect(OpGroupJump):
    @classmethod
    def opcode(cls) -> int:
        return 4

    @classmethod
    def c_name(cls) -> str:
        return "JPD"


class OpJumpIndirect(OpGroupJump):
    @classmethod
    def opcode(cls) -> int:
        return 5

    @classmethod
    def c_name(cls) -> str:
        return "JPI"


class OpJumpMarkDirect(OpGroupJump):
    @classmethod
    def opcode(cls) -> int:
        return 6

    @classmethod
    def c_name(cls) -> str:
        return "JMD"


class OpJumpMarkIndirect(OpGroupJump):
    @classmethod
    def opcode(cls) -> int:
        return 7

    @classmethod
    def c_name(cls) -> str:
        return "JMI"


class OpGroupBits(ABC):
    def __init__(self, digit: int):
        if digit < 0 or digit > 7:
            raise ValueError(digit)

        self._digit = digit

    def to_code(self) -> list[int]:
        code = OctalByte(0)
        code.set_high(self.opcode())
        code.set_mid(self._digit)
        code.set_low(2)

        return [code.value()]

    @classmethod
    @abstractmethod
    def opcode(cls) -> int:
        pass

    @classmethod
    @abstractmethod
    def c_name(cls) -> str:
        pass


class OpSetTo0(OpGroupBits):
    @classmethod
    def opcode(cls) -> int:
        return 0

    @classmethod
    def c_name(cls) -> str:
        return "SetTo0"


class OpSetTo1(OpGroupBits):
    @classmethod
    def opcode(cls) -> int:
        return 1

    @classmethod
    def c_name(cls) -> str:
        return "SetTo1"


class OpSkipOn0(OpGroupBits):
    @classmethod
    def opcode(cls) -> int:
        return 2

    @classmethod
    def c_name(cls) -> str:
        return "SkipOn0"


class OpSkipOn1(OpGroupBits):
    @classmethod
    def opcode(cls) -> int:
        return 3

    @classmethod
    def c_name(cls) -> str:
        return "SkipOn1"

class OpGroupShiftRotate(ABC):
    @classmethod
    def valid_places(cls) -> list[int]:
        return [1,2,3,4]

    def __init__(self, register: RegisterShiftRotate, places: int):
        if places not in self.valid_places():
            raise ValueError(places)

        self._register = register
        self._places = places

    def to_code(self) -> list[int]:
        if self._places != 4:
            place_code = self._places
        else:
            place_code = 0
        code = OctalByte(0)
        code.set_high(self.opcode())
        code.set_mid(self._register.value + place_code)
        code.set_low(1)

        return [code.value()]

    @classmethod
    @abstractmethod
    def opcode(cls) -> int:
        pass

    @classmethod
    @abstractmethod
    def c_name(cls) -> str:
        pass


class OpRightShift(OpGroupShiftRotate):
    @classmethod
    def opcode(cls) -> int:
        return 0

    @classmethod
    def c_name(cls) -> str:
        return "Right shift"

class OpRightRotate(OpGroupShiftRotate):
    @classmethod
    def opcode(cls) -> int:
        return 1

    @classmethod
    def c_name(cls) -> str:
        return "Right rotate"

class OpLeftShift(OpGroupShiftRotate):
    @classmethod
    def opcode(cls) -> int:
        return 2

    @classmethod
    def c_name(cls) -> str:
        return "Left shift"

class OpLeftRotate(OpGroupShiftRotate):
    @classmethod
    def opcode(cls) -> int:
        return 3

    @classmethod
    def c_name(cls) -> str:
        return "Left rotate"

class OpGroupMisc(ABC):
    def to_code(self) -> list[int]:
        code = OctalByte(0)
        code.set_high(self.opcode())
        code.set_mid(0)
        code.set_low(0)

        return [code.value()]

    @classmethod
    @abstractmethod
    def opcode(cls) -> int:
        pass

    @classmethod
    @abstractmethod
    def c_name(cls) -> str:
        pass

class OpHalt(OpGroupMisc):
    @classmethod
    def opcode(cls) -> int:
        return 0

    @classmethod
    def c_name(cls) -> str:
        return "Halt"

class OpNoop(OpGroupMisc):
    @classmethod
    def opcode(cls) -> int:
        return 2

    @classmethod
    def c_name(cls) -> str:
        return "Noop"

leader = " " * 4
image = [0x0]*256
image[0o003] = 0o004 # PC = 004
image[0o004] = 0o000 # HALT

mem_addr = 0o005
for op_class in [OpAdd, OpSub, OpLoad, OpStore]:
    for reg in list(Register):
        print(f"{leader}// {op_class.c_name()} {reg.c_name()}")
        for addressing in list(Addressing):
            opcode = op_class(register=reg, addressing=addressing)
            code = opcode.to_code()[0]

            image[mem_addr] = code
            print(
                f"{leader}memory[{mem_addr:0>4o}] = {code:0>4o};  // {addressing.c_name()}"
            )
            mem_addr += 1
        else:
            print("")

# mem_addr = 0o101
for op_class in [OpOr, OpAnd, OpLneg]:
    print(f"{leader}// {op_class.c_name()}")
    for addressing in list(Addressing):
        opcode = op_class(addressing=addressing)
        code = opcode.to_code()[0]

        image[mem_addr] = code
        print(
            f"{leader}memory[{mem_addr:0>4o}] = {code:0>4o};  // {addressing.c_name()}"
        )
        mem_addr += 1
    else:
        print("")

# mem_addr = 0o120
for op_class in [OpJumpDirect, OpJumpIndirect, OpJumpMarkDirect, OpJumpMarkIndirect]:
    for check in list(JumpCheck):
        print(f"{leader}// {op_class.c_name()} {check.c_name()}")
        for condition in list(JumpCondition):
            opcode = op_class(check=check, condition=condition)
            code = opcode.to_code()[0]

            if(mem_addr == 0o200):
                mem_addr = 0o204
            image[mem_addr] = code
            print(
                f"{leader}memory[{mem_addr:0>4o}] = {code:0>4o};  // {condition.c_name()}"
            )
            mem_addr += 1
        else:
            print("")

# mem_addr = 0o244
for op_class in [OpSetTo0, OpSetTo1, OpSkipOn0, OpSkipOn1]:
    print(f"{leader}// {op_class.c_name()}")
    for digit in range(8):
        opcode = op_class(digit=digit)
        code = opcode.to_code()[0]

        image[mem_addr] = code
        print(
            f"{leader}memory[{mem_addr:0>4o}] = {code:0>4o};  // {digit}"
        )
        mem_addr += 1
    else:
        print("")

# mem_addr = 0o304
for op_class in [OpRightShift, OpRightRotate, OpLeftShift, OpLeftRotate]:
    for register in list(RegisterShiftRotate):
        print(f"{leader}// {op_class.c_name()} {register.c_name()}")
        for places in OpGroupShiftRotate.valid_places():
            opcode = op_class(register=register, places=places)
            code = opcode.to_code()[0]

            image[mem_addr] = code
            print(
                f"{leader}memory[{mem_addr:0>4o}] = {code:0>4o};  // {places}"
            )
            mem_addr += 1
        else:
            print("")

# mem_addr = 0o344
for op_class in [OpHalt, OpNoop]:
    print(f"{leader}// {op_class.c_name()}")
    opcode = op_class()
    code = opcode.to_code()[0]

    image[mem_addr] = code
    print(
        f"{leader}memory[{mem_addr:0>4o}] = {code:0>4o};"
    )
    mem_addr += 1
    print("\n")


print("const uint8_t kImageInstructions[MEM_SIZE] = {", end='')
for index, code in enumerate(image):
    if (index)%16 == 0:
        print(f'\n{leader}', end='')
    print(f"{code:0>4o},", end='')
print('\n};')



demoImage = [0x0]*256
# All comments in octal
demoImage[0o003] = 0o004 # PC = 004
demoImage[0o004] = 0o023 # LOAD A C=123
demoImage[0o005] = 0o123
demoImage[0o006] = 0o123 # LOAD B C=234
demoImage[0o007] = 0o234
demoImage[0o010] = 0o223 # LOAD X C=345
demoImage[0o011] = 0o345
print("const uint8_t kImageDemo[MEM_SIZE] = {", end='')
for index, code in enumerate(demoImage):
    if (index)%16 == 0:
        print(f'\n{leader}', end='')
    print(f"{code:0>4o},", end='')
print('\n};')