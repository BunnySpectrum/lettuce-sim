# Placement-new polymorphic destruction

This experiment compares two ways to destroy a polymorphic object constructed
in caller-owned storage:

- `virtual_destructor.cpp` uses a conventional virtual destructor. Its
  Energia-style global `operator delete` calls `free`, so the compiler-generated
  deleting destructors pull the allocator into the linked image.
- `destroy_hook.cpp` keeps the base destructor non-virtual and exposes a
  virtual `destroy()` function. Each derived implementation invokes its exact
  destructor without requesting heap deallocation.

Both examples construct `Derived` with placement new and have its destructor
write `kCleanupValue` to `cleanup_observed`. `exercise()` is out of line so the
compiler must perform polymorphic dispatch rather than proving the concrete
type in `main`. Each file also supplies the tiny `__cxa_pure_virtual` stub that
Energia normally provides, allowing the standalone ELF to link.

## Build

Run these commands from the project root after PlatformIO has installed the
`toolchain-timsp430` package:

```sh
mkdir -p /tmp/placement-destruction

~/.platformio/packages/toolchain-timsp430/bin/msp430-g++ \
  -mmcu=msp430g2553 -std=c++0x -Os -fno-exceptions \
  -ffunction-sections -fdata-sections -c \
  test/placement_destruction/virtual_destructor.cpp \
  -o /tmp/placement-destruction/virtual_destructor.o

~/.platformio/packages/toolchain-timsp430/bin/msp430-g++ \
  -mmcu=msp430g2553 -std=c++0x -Os -fno-exceptions \
  -ffunction-sections -fdata-sections -c \
  test/placement_destruction/destroy_hook.cpp \
  -o /tmp/placement-destruction/destroy_hook.o

~/.platformio/packages/toolchain-timsp430/bin/msp430-gcc \
  -mmcu=msp430g2553 -Wl,--gc-sections \
  /tmp/placement-destruction/virtual_destructor.o \
  -o /tmp/placement-destruction/virtual_destructor.elf

~/.platformio/packages/toolchain-timsp430/bin/msp430-gcc \
  -mmcu=msp430g2553 -Wl,--gc-sections \
  /tmp/placement-destruction/destroy_hook.o \
  -o /tmp/placement-destruction/destroy_hook.elf
```

The separate `g++` compile and `gcc` link match PlatformIO's behavior for this
toolchain; linking with `g++` would request an unavailable `libstdc++`.

## Confirm the RAM and allocator difference

```sh
~/.platformio/packages/toolchain-timsp430/bin/msp430-size -A \
  /tmp/placement-destruction/virtual_destructor.elf \
  /tmp/placement-destruction/destroy_hook.elf

~/.platformio/packages/toolchain-timsp430/bin/msp430-nm \
  -S --size-sort --demangle \
  /tmp/placement-destruction/virtual_destructor.elf \
  /tmp/placement-destruction/destroy_hook.elf
```

The virtual-destructor image should contain `operator delete(void*)`, `free`,
`malloc`, and the allocator's local `once.*` byte in `.bss`. The destroy-hook
image should contain none of those allocator symbols. With this toolchain, the
expected `.bss` sizes are four bytes for the virtual-destructor image and two
bytes for the destroy-hook image. Section alignment makes the one-byte
allocator state increase linked RAM by two bytes, matching the production
firmware's observed increase from 406 to 408 bytes.

Both images should contain `Derived::~Derived()`, `cleanup_observed`, and a
polymorphic call from `exercise()`. This confirms that the hook retains derived
cleanup while avoiding heap-deallocation support.

To inspect the calls directly:

```sh
~/.platformio/packages/toolchain-timsp430/bin/msp430-objdump \
  -d -C /tmp/placement-destruction/virtual_destructor.elf

~/.platformio/packages/toolchain-timsp430/bin/msp430-objdump \
  -d -C /tmp/placement-destruction/destroy_hook.elf
```

Neither example calls `delete` on the placement-new pointer. The virtual
destructor's deleting-destructor entries are emitted as part of its ABI even
though the program only makes an explicit destructor call.
