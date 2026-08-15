#!/usr/bin/env bash

# Smoke-test a built plugin against representative bad and good header forms.

set -euo pipefail

script_dir=$(cd "$(dirname "$0")" && pwd)
clang_tidy=${CLANG_TIDY:-/opt/homebrew/opt/llvm/bin/clang-tidy}
plugin=${1:-$script_dir/build-plugin/LettuceTidy.dylib}

if [[ ! -x "$clang_tidy" ]]; then
    printf 'clang-tidy not found: %s\n' "$clang_tidy" >&2
    exit 2
fi

if [[ ! -f "$plugin" ]]; then
    printf 'Plugin not found: %s\n' "$plugin" >&2
    exit 2
fi

output=$(
    "$clang_tidy" \
        --load="$plugin" \
        --checks=-*,lettuce-header-internal-object \
        --header-filter='.*' \
        "$script_dir/tests/test.cpp" \
        -- -std=c++17 -I"$script_dir/tests" 2>&1
)

printf '%s\n' "$output"

for expected in HeaderPoint RuntimeInteger LiteralInteger; do
    if ! printf '%s\n' "$output" | grep -q "$expected"; then
        printf 'Expected diagnostic for %s was not found.\n' "$expected" >&2
        exit 1
    fi
done

for unexpected in DeclaredOnlyPoint DeclaredOnlyInteger InlineInteger; do
    if printf '%s\n' "$output" | grep -q "$unexpected"; then
        printf 'Unexpected diagnostic for %s was found.\n' "$unexpected" >&2
        exit 1
    fi
done

printf 'Plugin smoke test passed.\n'
