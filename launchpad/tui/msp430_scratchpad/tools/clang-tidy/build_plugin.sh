#!/usr/bin/env bash

# Build LettuceTidy against a complete LLVM installation. The resulting plugin
# must be loaded by clang-tidy from the same LLVM installation.

set -euo pipefail

script_dir=$(cd "$(dirname "$0")" && pwd)
build_dir=${BUILD_DIR:-$script_dir/build-plugin}
llvm_root=${LLVM_ROOT:-}

if [[ -z "$llvm_root" ]] && command -v brew >/dev/null 2>&1; then
    llvm_root=$(brew --prefix llvm 2>/dev/null || true)
fi

if [[ -z "$llvm_root" ]]; then
    printf 'LLVM installation not found; set LLVM_ROOT.\n' >&2
    exit 2
fi

for executable in clang clang++ clang-tidy; do
    if [[ ! -x "$llvm_root/bin/$executable" ]]; then
        printf 'Required LLVM executable not found: %s/bin/%s\n' \
            "$llvm_root" "$executable" >&2
        exit 2
    fi
done

cmake \
    -S "$script_dir" \
    -B "$build_dir" \
    -DCMAKE_C_COMPILER="$llvm_root/bin/clang" \
    -DCMAKE_CXX_COMPILER="$llvm_root/bin/clang++" \
    -DLLVM_DIR="$llvm_root/lib/cmake/llvm" \
    -DClang_DIR="$llvm_root/lib/cmake/clang"

cmake --build "$build_dir" -j2

printf 'Built LettuceTidy in %s\n' "$build_dir"

