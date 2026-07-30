from __future__ import annotations
import enum
from abc import ABC, abstractmethod


class Register(enum.Enum):
    kA = 0
    kB = 1
    kX = 2

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


leader = " " * 4
# mem_addr = 0o20
# for op_class in [OpAdd, OpSub, OpLoad, OpStore]:
#     for reg in list(Register):
#         print(f"{leader}// {op_class.c_name()} {reg.c_name()}")
#         for addressing in list(Addressing):
#             opcode = op_class(register=reg, addressing=addressing)
#             code = opcode.to_code()[0]

#             print(
#                 f"{leader}memory[{mem_addr:0>4o}] = {code:0>4o};  // {addressing.c_name()}"
#             )
#             mem_addr += 1
#         else:
#             print("")

# mem_addr = 0o114
# for op_class in [OpOr, OpAnd, OpLneg]:
#     print(f"{leader}// {op_class.c_name()}")
#     for addressing in list(Addressing):
#         opcode = op_class(addressing=addressing)
#         code = opcode.to_code()[0]

#         print(
#             f"{leader}memory[{mem_addr:0>4o}] = {code:0>4o};  // {addressing.c_name()}"
#         )
#         mem_addr += 1
#     else:
#         print("")

mem_addr = 0o133
for op_class in [OpJumpDirect, OpJumpIndirect, OpJumpMarkDirect, OpJumpMarkIndirect]:
    for check in list(JumpCheck):
        print(f"{leader}// {op_class.c_name()} {check.c_name()}")
        for condition in list(JumpCondition):
            opcode = op_class(check=check, condition=condition)
            code = opcode.to_code()[0]

            print(
                f"{leader}memory[{mem_addr:0>4o}] = {code:0>4o};  // {condition.c_name()}"
            )
            mem_addr += 1
        else:
            print("")
