# Firmware size comparison

`binsize` builds the configured PlatformIO environments, reads each generated
`sizedata.json`, and prints a compact comparison of the sections that consume
MCU flash or RAM.

From the project root:

```sh
go -C tools/track run ./cmd/binsize -project ../..
```

The default comparison is:

- `release` using the `lpmsp430g2553` PlatformIO environment
- `debug-full` using the `debug-full` PlatformIO environment

The command writes a combined report to
`.pio/build/size-comparison.json`. To render existing build data without
rebuilding:

```sh
go -C tools/track run ./cmd/binsize -project ../.. -no-build
```

Additional environments can be supplied as comma-separated
`label=environment` pairs:

```sh
go -C tools/track run ./cmd/binsize -project ../.. \
    -builds release=lpmsp430g2553,debug-minimal=debug-minimal,debug-full=debug-full
```

## Stack-path analysis

`stackusage` measures explicit MSP430 function frames in object or ELF
disassembly and totals them along a supplied call path. See
[`docs/stack_usage.md`](../../docs/stack_usage.md) for the current
`App::app_decode` analysis and a complete invocation.
