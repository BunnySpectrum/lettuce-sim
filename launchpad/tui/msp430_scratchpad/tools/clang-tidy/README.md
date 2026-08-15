# Lettuce clang-tidy check

`lettuce-header-internal-object` warns about namespace-scope `const` objects
defined in project headers with internal linkage. Including such a header from
multiple translation units may create a separate object in every translation
unit.

The diagnostic distinguishes between:

- objects requiring dynamic initialization, which will normally require
  per-translation-unit storage and startup work; and
- constant-initialized objects, which may require separate storage when they
  are odr-used.

The check ignores `extern` declarations, C++17 inline variables, system
headers, local statics, static class data members, and macro-generated
declarations. It intentionally reports potential duplication; use the object
and map tools in the parent directory to determine which copies were actually
emitted and retained.

## Prerequisite

Install a complete LLVM distribution containing clang-tidy and development
headers. On macOS:

```bash
brew install llvm
```

The plugin must be built and run with the same LLVM installation. PlatformIO's
bundled LLVM 15 clang-tidy is not used because it does not include matching
development headers and has a different plugin ABI.

## Build and test

From the project root:

```bash
tools/clang-tidy/build_plugin.sh
tools/clang-tidy/test_plugin.sh
```

For a non-Homebrew LLVM installation, set its prefix explicitly:

```bash
LLVM_ROOT=/path/to/llvm tools/clang-tidy/build_plugin.sh
CLANG_TIDY=/path/to/llvm/bin/clang-tidy \
    tools/clang-tidy/test_plugin.sh
```

The smoke test verifies warnings for a dynamically initialized class object, a
runtime-initialized integer, and a constant-initialized integer. It also
verifies that `extern` declarations and inline variables are accepted.

## Run with PlatformIO

The PlatformIO adapter generates the standard compilation database and invokes
the portable runner:

```bash
tools/run_platformio_header_lint.sh
```

Choose another PlatformIO environment with:

```bash
tools/run_platformio_header_lint.sh -e environment-name
```

PlatformIO is only responsible for generating `compile_commands.json`. The
actual analysis uses the project-selected LLVM installation and plugin.

## Run without PlatformIO

Any build system may generate a standard `compile_commands.json`. Pass its
directory to the generic runner:

```bash
tools/run_header_internal_object_check.sh \
    --build-path path/to/compilation-database
```

To analyze selected translation units only, append their paths:

```bash
tools/run_header_internal_object_check.sh \
    --build-path path/to/compilation-database \
    src/app.cpp src/main.cpp
```

Override tool locations when necessary:

```bash
tools/run_header_internal_object_check.sh \
    --clang-tidy /path/to/clang-tidy \
    --plugin /path/to/LettuceTidy.so \
    --build-path path/to/compilation-database
```

The runner analyzes project translation units, reports each header diagnostic
once even when several translation units include it, and treats findings as
errors for CI purposes.

## Exit status

- `0`: analysis completed without findings
- `1`: the check found risky definitions or clang could not analyze a source
- `2`: missing tool, plugin, database, or invalid command-line usage

