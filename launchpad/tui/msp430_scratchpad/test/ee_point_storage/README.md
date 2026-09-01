# `EePoint` storage at `-O0`

This experiment demonstrates how MSP430 GCC 4.6.3 places three forms of a
two-byte `constexpr` point when optimization is disabled:

| Source | Point design | Expected section |
| --- | --- | --- |
| `constructor_class.cpp` | Private fields, constructor, accessors | `.data` |
| `constructor_struct.cpp` | Public fields, but still has a constructor | `.data` |
| `aggregate_struct.cpp` | Public fields and no constructor | `.rodata` |

This distinguishes the relevant language features from the `class`/`struct`
keyword itself. In C++, those keywords primarily change the default member
visibility. The constructor-free aggregate is what allows this compiler to
keep the object in read-only storage at `-O0`.

## Reproduce

Run these commands from the project root after PlatformIO has installed the
`toolchain-timsp430` package:

```sh
mkdir -p /tmp/ee-point-storage

~/.platformio/packages/toolchain-timsp430/bin/msp430-g++ \
  -mmcu=msp430g2553 -std=c++0x -O0 -c \
  test/ee_point_storage/constructor_class.cpp \
  -o /tmp/ee-point-storage/constructor_class.o

~/.platformio/packages/toolchain-timsp430/bin/msp430-g++ \
  -mmcu=msp430g2553 -std=c++0x -O0 -c \
  test/ee_point_storage/constructor_struct.cpp \
  -o /tmp/ee-point-storage/constructor_struct.o

~/.platformio/packages/toolchain-timsp430/bin/msp430-g++ \
  -mmcu=msp430g2553 -std=c++0x -O0 -c \
  test/ee_point_storage/aggregate_struct.cpp \
  -o /tmp/ee-point-storage/aggregate_struct.o
```

Inspect the section sizes:

```sh
~/.platformio/packages/toolchain-timsp430/bin/msp430-size -A \
  /tmp/ee-point-storage/constructor_class.o \
  /tmp/ee-point-storage/constructor_struct.o \
  /tmp/ee-point-storage/aggregate_struct.o
```

The first two object files should each have two bytes in `.data`. The aggregate
object file should instead have two bytes in `.rodata` and zero bytes in
`.data`.

Inspect the point symbols directly:

```sh
~/.platformio/packages/toolchain-timsp430/bin/msp430-nm \
  -S --size-sort --demangle /tmp/ee-point-storage/*.o
```

The symbol type printed before `kPoint` should be lowercase `d` for the two
constructor cases and lowercase `r` for the aggregate case. Lowercase means
the symbol has translation-unit-local linkage; `d` means initialized writable
data, while `r` means read-only data.

For comparison, repeat the compilation commands with `-Os`. The optimizer can
fold the point values into the generated code and may remove the point symbols
entirely.

