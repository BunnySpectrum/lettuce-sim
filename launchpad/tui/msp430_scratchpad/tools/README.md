# Duplicate local object checks

These tools detect local C++ objects that are emitted into `.data`, `.bss`,
`.noinit`, or `.rodata` by more than one translation unit. This commonly
happens when a namespace-scope `const` object is defined in a header included
by multiple source files. Writable sections consume RAM; `.rodata` normally
consumes flash.

Both tools focus on mangled names beginning with `_ZL`, the Itanium C++ ABI
prefix used for namespace-scope entities with internal linkage. Names are
demangled in the report by default.

## Check object files before linking

`find_duplicate_object_symbols.sh` recursively inspects `.o` files and does not
require a linker map. Prefer passing only the directories containing your own
objects so constants from PlatformIO frameworks and third-party libraries do
not dominate the report. For this project, scan both the application and its
project-library objects:

```bash
tools/find_duplicate_object_symbols.sh \
    .pio/build/lpmsp430g2553/src \
    .pio/build/lpmsp430g2553/libdab
```

Passing `.pio/build/lpmsp430g2553` instead scans every object in the build,
including the HAL and framework. That broader result is valid, but usually much
noisier after `.rodata` is included.

Individual object files are also accepted:

```bash
tools/find_duplicate_object_symbols.sh \
    .pio/build/lpmsp430g2553/src/app.cpp.o \
    .pio/build/lpmsp430g2553/src/main.cpp.o
```

To retain mangled names in the report:

```bash
tools/find_duplicate_object_symbols.sh --no-demangle \
    .pio/build/lpmsp430g2553/src
```

The default tool commands may be overridden when using another target:

```bash
OBJDUMP=objdump CXXFILT=c++filt \
    tools/find_duplicate_object_symbols.sh path/to/objects
```

## Check objects that survived the final link

`find_duplicate_map_symbols.sh` inspects a GNU `ld` linker map and includes the
final address and defining object for every copy:

```bash
tools/find_duplicate_map_symbols.sh \
    .pio/build/lpmsp430g2553/output.map
```

To retain mangled names:

```bash
tools/find_duplicate_map_symbols.sh --no-demangle \
    .pio/build/lpmsp430g2553/output.map
```

The map parser expects per-variable input sections produced by
`-fdata-sections`, as used by this project. It ignores discarded input sections
and examines only the final `.data`, `.bss`, `.noinit`, and `.rodata` output
sections.

## Exit status

Both tools use lint-friendly exit statuses:

- `0`: no duplicate local objects found
- `1`: one or more duplicates found
- `2`: invalid arguments, missing input, or a required command was not found

Use `--help` to show the command-line summary for either tool.

## Follow a symbol through all three checks

`show_duplicate_symbol_pipeline.sh` is a minimal text UI that runs the source
lint and both artifact checks, then correlates their findings by demangled
symbol name:

```bash
tools/show_duplicate_symbol_pipeline.sh \
    --build-path . \
    --map .pio/build/lpmsp430g2553/output.map \
    .pio/build/lpmsp430g2553/src \
    .pio/build/lpmsp430g2553/libdab
```

Each row shows whether the source pattern was warned about, duplicate objects
were emitted before linking, and duplicate objects survived into the final
image. Rows impacting the final image (`IMPACTING = Y`) appear first. A `-`
means that stage did not report a duplicate; it does not prove the symbol was
entirely absent from that stage. `ORIGIN` gives the header location reported by
the source lint; artifact-only findings show `-` because the object and map
checks do not currently provide source locations.

RAM and flash usage from the final ELF appear above the table. The ELF defaults
to `firmware.elf` in the map file's directory; pass `--elf FILE` for another
layout. The pipeline first runs `platformio run --target sizedata` silently so
the build artifacts and usage are current. Override the PlatformIO and target
size commands with `PLATFORMIO` and `SIZE` if needed.

## Source-level clang-tidy check

The `clang-tidy/` directory contains the complementary source-level rule. It
warns before linking when an internal-linkage `const` object is defined in a
header and may be emitted once per translation unit.

Build and smoke-test the plugin:

```bash
tools/clang-tidy/build_plugin.sh
tools/clang-tidy/test_plugin.sh
```

Run it using PlatformIO's generated compilation database:

```bash
tools/run_platformio_header_lint.sh
```

Or use a compilation database produced by another build system:

```bash
tools/run_header_internal_object_check.sh \
    --build-path path/to/compilation-database
```

See `clang-tidy/README.md` for design details and tool overrides.
